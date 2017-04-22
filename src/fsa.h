#ifndef FSA_H_7KZTYN9I
#define FSA_H_7KZTYN9I

#define SYM_ANY (-1)
#define SYM_VARNAME (-2)
#define SYM_SPACE (-3)
#define SYM_END_1 (-4)

/*
 * simple fsa, without explicit accepted states.
 * implementat as an array of hashtables, i.e. blocks of size 128*sizeof(int)
 */

typedef struct {
  int no_states : 16;
  int cur_st : 16;
  int *states;
} fsa_t;

typedef enum {
  DEFAULT,
  BCKSLSH_DF_IN,
  BCKSLSH_DF_OUT,
  DOLLAR,
  VARIABLE,
  SQUOTE_BEGIN,
  SQUOTE,
  SQUOTE_END,
  DQUOTE_BEGIN,
  DQUOTE,
  DQUOTE_END,
  BCKSLSH_DQ_IN,
  BCKSLSH_DQ_OUT,
  WHITESPACE,
  AMPERSAND,
  CMD_END_1,
  CMD_END_2,
  ARGS_INVALID,
  NO_ARGS_STATES,

  STREAM_INITIAL = DEFAULT,
  STREAM_INPUT_TYPE,
  INPUT_ACTION,
  INPUT_INTERMEDIATE_ACTION,
  INPUT_INLINE_ACTION,
  OUTPUT_WRITE_ACTION,
  OUTPUT_APPEND_ACTION,
  STREAM_OUTPUT_AMPERSAND,
  STREAM_OUTPUT_TYPE,
  STREAM_FILENAME,
  STREAM_INVALID,
  NO_STREAM_STATES,

  CMDOP_INIT = DEFAULT,
  CMDOP_VERT,
  CMDOP_VERT2,
  CMDOP_AMPERS1,
  CMDOP_AMPERS2,
  CMDOP_NOOP,
  CMDOP_END_0,
  CMDOP_END_1,
  CMDOP_INVALID,
  NO_CMDOP_STATES
} FSA_STATE;

typedef void(char_visitor(char c));

fsa_t create_fsa(int no_states);
void connect(fsa_t f, int st_from, char tr, int st_to);
void connect_s(fsa_t f, int st_from, const char *str, int st_to);
void set_default_tr(fsa_t f, int st_from, int st_to);
void copy_state(fsa_t f, int st_from, int st_to);

void set_args_fsa(fsa_t *f);
void set_stream_fsa(fsa_t *f);
void set_cmop_fsa(fsa_t *f);
void fsa_spit_sample_stream();
void fsa_spit_sample_cmdop();
void fsa_spit_sample_args();

void process_char(fsa_t *f, char c);
void parse_text(fsa_t *f, const char *text);

fsa_t make_args_fsa();
fsa_t make_stream_fsa();
fsa_t make_cmdop_fsa();

void clear_fsa(fsa_t f);

#endif
