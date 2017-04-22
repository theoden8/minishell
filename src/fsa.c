#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#include "fsa.h"
#include "logger.h"

// allocate fsa states and transitions
fsa_t create_fsa(int no_states) {
  fsa_t f;
  f.no_states = no_states;
  f.states = malloc(sizeof(int) * no_states * 128);
  assert(f.states != NULL);
  for(int i = 0; i < no_states; ++i) {
    for(int j = 0; j < 128; ++j) {
      f.states[i * 128 + j] = i;
    }
  }
  return f;
}

// conncet two states with a symbol
void connect(fsa_t f, int st_from, char tr, int st_to) {
  switch(tr) {
    case SYM_ANY:;
      for(unsigned char c = 0; c < 128; ++c)
        connect(f, st_from, c, st_to);
      break;
    case SYM_VARNAME:;
      for(const char *c = "qwertyuiopasdfghjklzxcvbnm!?$"; *c != '\0'; ++c) {
        connect(f, st_from, *c, st_to);
      }
    break;
    case SYM_SPACE:;
      for(unsigned char c = 0; c < 128; ++c)
        if(isspace(c))
          connect(f, st_from, c, st_to);
    break;
    case SYM_END_1:;
      for(const char *c = "\n|;)}"; *c != '\0'; ++c) {
        connect(f, st_from, *c, st_to);
      }
    break;
    default:
      assert(tr >= 0);
      f.states[st_from * 128 + tr] = st_to;
    break;
  }
}

// connect two states by several symbols
void connect_s(fsa_t f, int st_from, const char *str, int st_to) {
  for(const char *c = str; *c != '\0'; ++c)
    connect(f, st_from, *c, st_to);
}

// copy all transitions from ont state to another
void copy_state(fsa_t f, int st_from, int st_to) {
  memcpy(f.states + 128 * st_to, f.states + 128 * st_from, 128 * sizeof(int));
}

// configure fsa for parsing arguments
void set_args_fsa(fsa_t *f) {
  f->cur_st = DEFAULT;
  connect(*f, DEFAULT, SYM_END_1, CMD_END_1);
  connect(*f, DEFAULT, '"', DQUOTE_BEGIN);
  connect(*f, DEFAULT, '\'', SQUOTE_BEGIN);
  connect(*f, DEFAULT, '\\', BCKSLSH_DF_IN);
  connect(*f, DEFAULT, ' ', WHITESPACE);
  connect(*f, DEFAULT, '\t', WHITESPACE);
  connect(*f, DEFAULT, '$', DOLLAR);
  connect(*f, DEFAULT, '&', AMPERSAND);
  copy_state(*f, DEFAULT, DOLLAR);
  connect(*f, DOLLAR, SYM_VARNAME, VARIABLE);
  copy_state(*f, DEFAULT, VARIABLE);
  connect(*f, VARIABLE, SYM_VARNAME, VARIABLE);

  connect(*f, BCKSLSH_DF_IN, SYM_ANY, BCKSLSH_DF_OUT);
  connect(*f, BCKSLSH_DF_OUT, SYM_ANY, DEFAULT);

  connect(*f, DQUOTE_BEGIN, SYM_ANY, DQUOTE);
  connect(*f, DQUOTE_BEGIN, '"', DQUOTE_END);
  connect(*f, DQUOTE, '\\', BCKSLSH_DQ_IN);
  connect(*f, DQUOTE, '"', DQUOTE_END);
  connect(*f, DQUOTE_END, SYM_ANY, DEFAULT);
  connect(*f, DQUOTE_END, SYM_END_1, CMD_END_1);
  connect(*f, DQUOTE_END, '&', AMPERSAND);
  connect(*f, DQUOTE_END, '"', DQUOTE_BEGIN);
  connect(*f, DQUOTE_END, '\'', SQUOTE_BEGIN);
  connect(*f, DQUOTE_END, ' ', WHITESPACE);

  connect(*f, BCKSLSH_DQ_IN, SYM_ANY, BCKSLSH_DQ_OUT);
  connect(*f, BCKSLSH_DQ_OUT, '\0', DQUOTE);
  connect(*f, BCKSLSH_DQ_OUT, '"', DEFAULT);

  connect(*f, SQUOTE_BEGIN, SYM_ANY, SQUOTE);
  connect(*f, SQUOTE_BEGIN, '\'', SQUOTE_END);
  connect(*f, SQUOTE, '\'', SQUOTE_END);
  copy_state(*f, DQUOTE_END, SQUOTE_END);

  connect(*f, WHITESPACE, SYM_ANY, DEFAULT);
  connect(*f, WHITESPACE, SYM_END_1, CMD_END_1);
  connect(*f, WHITESPACE, ' ', WHITESPACE);
  connect(*f, WHITESPACE, '\t', WHITESPACE);
  connect(*f, WHITESPACE, '"', DQUOTE_BEGIN);
  connect(*f, WHITESPACE, '\'', SQUOTE_BEGIN);
  connect(*f, WHITESPACE, '$', DOLLAR);
  connect(*f, WHITESPACE, '&', AMPERSAND);
  connect(*f, WHITESPACE, '\\', BCKSLSH_DF_IN);

  connect(*f, AMPERSAND, SYM_ANY, CMD_END_1);
  connect(*f, AMPERSAND, '>', DEFAULT);
  connect(*f, AMPERSAND, '1', DEFAULT);
  connect(*f, AMPERSAND, '2', DEFAULT);
  connect(*f, AMPERSAND, '&', CMD_END_2);
}

// configure fsa for parsing streams (after parsing args)
void set_stream_fsa(fsa_t *f) {
  f->cur_st = STREAM_INITIAL;
  connect(*f, STREAM_INITIAL, SYM_ANY, STREAM_INVALID);
  connect(*f, STREAM_INITIAL, '>', OUTPUT_WRITE_ACTION);
  connect(*f, STREAM_INITIAL, '<', INPUT_ACTION);
  copy_state(*f, STREAM_INITIAL, STREAM_INPUT_TYPE);

  connect(*f, INPUT_ACTION, SYM_ANY, STREAM_FILENAME);
  connect(*f, INPUT_INLINE_ACTION, SYM_ANY, STREAM_FILENAME);
  connect(*f, OUTPUT_WRITE_ACTION, SYM_ANY, STREAM_FILENAME);
  connect(*f, OUTPUT_APPEND_ACTION, SYM_ANY, STREAM_FILENAME);

  connect(*f, STREAM_INITIAL, '1', STREAM_INPUT_TYPE);
  connect(*f, STREAM_INITIAL, '2', STREAM_INPUT_TYPE);
  connect(*f, STREAM_INITIAL, '&', STREAM_INPUT_TYPE);

  connect(*f, INPUT_ACTION, '<', INPUT_INTERMEDIATE_ACTION);
  connect(*f, INPUT_INTERMEDIATE_ACTION, SYM_ANY, STREAM_INVALID);
  connect(*f, INPUT_INTERMEDIATE_ACTION, '<', INPUT_INLINE_ACTION);
  connect(*f, OUTPUT_WRITE_ACTION, '>', OUTPUT_APPEND_ACTION);
  connect(*f, OUTPUT_WRITE_ACTION, '&', STREAM_OUTPUT_AMPERSAND);
  connect(*f, OUTPUT_APPEND_ACTION, '&', STREAM_OUTPUT_AMPERSAND);

  connect(*f, STREAM_OUTPUT_AMPERSAND, SYM_ANY, STREAM_INVALID);
  connect(*f, STREAM_OUTPUT_AMPERSAND, '1', STREAM_OUTPUT_TYPE);
  connect(*f, STREAM_OUTPUT_AMPERSAND, '2', STREAM_OUTPUT_TYPE);
  connect(*f, STREAM_OUTPUT_TYPE, SYM_ANY, STREAM_INVALID);

  connect(*f, STREAM_FILENAME, SYM_ANY, STREAM_FILENAME);
  connect(*f, STREAM_INVALID, SYM_ANY, STREAM_INVALID);
}

// configure fsa for parsing the symobl after command
void set_cmdop_fsa(fsa_t *f) {
  f->cur_st = CMDOP_INIT;
  connect(*f, CMDOP_INIT, SYM_ANY, CMDOP_INVALID);
  connect_s(*f, CMDOP_INIT, ";\n", CMDOP_NOOP);
  connect_s(*f, CMDOP_INIT, ")}", CMDOP_END_1);
  connect(*f, CMDOP_INIT, '|', CMDOP_VERT);
  connect(*f, CMDOP_INIT, '&', CMDOP_AMPERS1);

  connect(*f, CMDOP_VERT, SYM_ANY, CMDOP_END_1);
  connect(*f, CMDOP_VERT, '|', CMDOP_VERT2);
  connect(*f, CMDOP_VERT2, SYM_ANY, CMDOP_END_1);

  connect(*f, CMDOP_AMPERS2, SYM_ANY, CMDOP_END_1);
  connect(*f, CMDOP_AMPERS1, SYM_ANY, CMDOP_END_1);
  connect(*f, CMDOP_AMPERS1, '&', CMDOP_AMPERS2);

  connect(*f, CMDOP_NOOP, SYM_ANY, CMDOP_END_1);
}

// give a random example of a some arguments that look valid
void fsa_spit_sample_args() {
  fsa_t f = make_args_fsa();
  int len = 0;
  while(f.cur_st != CMD_END_1 && f.cur_st != CMD_END_2) {
    char random_move;
    do{random_move = 32 + rand() % (127-32);}while(f.states[f.cur_st * 128 + random_move] == ARGS_INVALID);
    ++len;
    printf("%c", random_move);
    process_char(&f, random_move);
    if(len > 100){printf("...");break;}
    fflush(stdout);
  }
  clear_fsa(f);
}

// gives a random example of a stream that looks valid
void fsa_spit_sample_stream() {
  int len = 0;
  fsa_t f = make_stream_fsa();
  while(f.cur_st != STREAM_OUTPUT_TYPE) {
    char random_move;
    do{random_move = 32 + rand() % (127-32);}while(f.states[f.cur_st * 128 + random_move]==STREAM_INVALID);
    ++len;
    printf("%c", random_move);
    process_char(&f, random_move);
    if(len>10){printf("...");break;}
    fflush(stdout);
  }
  putchar('\n');
  clear_fsa(f);
}

// give an example of how a command should be terminated (i.e. linked to the
// next program)
void fsa_spit_sample_cmdop() {
  fsa_t f = make_cmdop_fsa();
  while(1) {
    char random_move;
    do{random_move = 32 + rand() % (127-32);}while(f.states[f.cur_st * 128 + random_move]==CMDOP_INVALID);
    process_char(&f, random_move);
    if(f.cur_st == CMDOP_END_0 || f.cur_st == CMDOP_END_1)
      break;
    printf("%c", random_move);
    fflush(stdout);
  }
  putchar('\n');
  clear_fsa(f);
}

// fsa step triggered with a character
void process_char(fsa_t *f, char c) {
  assert(c >= 0);
  f->cur_st = f->states[f->cur_st * 128 + c];
  pr_log("[%c] : %d\n", c, f->cur_st);
}

// make fsa process a string character by character
void parse_text(fsa_t *f, const char *text) {
  for(const char *c = text; *c != '\0'; ++c) {
    process_char(f,*c);
  }
}

// allocate and configure args fsa
fsa_t make_args_fsa() {
  fsa_t f = create_fsa(NO_ARGS_STATES);
  set_args_fsa(&f);
  return f;
}

// allocate and configure stream fsa
fsa_t make_stream_fsa() {
  fsa_t f = create_fsa(NO_STREAM_STATES);
  set_stream_fsa(&f);
  return f;
}

// allocate and configure cmdop fsa
fsa_t make_cmdop_fsa() {
  fsa_t f = create_fsa(NO_CMDOP_STATES);
  set_cmdop_fsa(&f);
  return f;
}

// deallocate fsa
void clear_fsa(fsa_t f) {
  free(f.states);
}
