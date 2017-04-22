#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "executor.h"
#include "parser.h"
#include "fsa.h"
#include "buffer.h"
#include "logger.h"

// get a pointer to the buffer matching the current reading mode
buffer_t *get_cur_buf(simplecmd_reader *ar){return(ar->reading_mode==1)?&ar->streams:&ar->argvbuf;}

// repeated in the end and at whitespaces
void finish_empty_word_simplecmd(simplecmd_reader *ar) {
  if(ar->should_separate_args && ar->last_wlen == 0) {
    if(ar->reading_mode == 1 && ar->argc != 0) {
      fprintf(stderr, "error: cannot read or write to file with no name\n");
      exit(1);
    }
    split_buffer(get_cur_buf(ar));
    if(ar->reading_mode == 0) {
      ++ar->argc;
    }
  }
  ar->last_wlen = 0;
}

// split argvbuf under certainn conditions
void split_words_simplecmd(simplecmd_reader *ar) {
  if(ar->reading_mode == 0 && ar->should_separate_args) {
    split_buffer(&ar->argvbuf);
    ar->should_separate_args = false;
    ++ar->argc;
  }
}

// update reader of simpelcmd with a character
void read_simplecmd_character(simplecmd_reader *ar, const char c) {
  ar->last_state = ar->f.cur_st;
  process_char(&ar->f, c);
  if(ar->f.cur_st!=WHITESPACE) {
    if(ar->repeated_space)ar->repeated_space=true;
    if(ar->first_space)ar->first_space=false;
  }
  switch(ar->f.cur_st) {
    case WHITESPACE:;
      if(ar->repeated_space)break;else ar->repeated_space=true;
      ar->repeated_space = false;
      if(ar->first_space){ar->first_space=false;break;}
      finish_empty_word_simplecmd(ar);
      if(ar->reading_mode == 1 && last_ch(ar->streams) != PIPE_LEFT && last_ch(ar->streams) != PIPE_RIGHT) {
        ar->reading_mode = 0;
        break;
      }
      ar->should_separate_args = true;
    break;
    case DQUOTE_BEGIN:case SQUOTE_BEGIN:
    break;
    case DQUOTE_END:case SQUOTE_END:
    break;
    case AMPERSAND:;
      if(ar->reading_mode == 0) {
        addch_buffer(&ar->ending, c);
      } else if(ar->reading_mode == 1) {
        addch_buffer(&ar->streams, c);
      }
    break;
    case DEFAULT:;
      if(strchr("<>", c) != NULL) {
        char sym = (c == '<') ? PIPE_LEFT : (c == '>') ? PIPE_RIGHT : c;
        assert(is_pipe_sym(sym));
        char strmprev = last_ch(ar->streams);
        if(ar->reading_mode == 1 || (strmprev != sym && strmprev != EOF)) {
          split_buffer(&ar->streams);
        }
        if(sym == PIPE_RIGHT && ar->last_state == DEFAULT) {
          // check if & or 12 preceed in default state
          if(ar->reading_mode == 0
             && strchr("12", ar->last_char) != NULL
             && ar->last_char == last_ch(ar->argvbuf)
            )
          {
            addch_buffer(&ar->streams, pop_ch(&ar->argvbuf));
            if(last_ch(ar->argvbuf) == BUF_SEP) {
              pop_ch(&ar->argvbuf);
              --ar->argc;
            }
            ++ar->last_wlen;
          } else if(
            ar->last_char == '&'
            && ar->last_char == last_ch(ar->ending))
          {
            addch_buffer(&ar->streams, pop_ch(&ar->ending));
            ++ar->last_wlen;
          }
        } else if(sym == PIPE_LEFT);
        ar->reading_mode = 1;
        addch_buffer(&ar->streams, sym);
        ++ar->last_wlen;
        break;
      }
    case DQUOTE:;
    case BCKSLSH_DF_OUT:;
    case BCKSLSH_DQ_OUT:;
    case SQUOTE:;
      if(ar->f.cur_st == BCKSLSH_DF_OUT && c == '\n')break;
      if(ar->reading_mode == 0) {
        split_words_simplecmd(ar);
      } else if(ar->reading_mode == 1) {
        if(ar->should_separate_streams == false)
          ar->should_separate_streams = true;
      }
      addch_buffer(get_cur_buf(ar), c);
      ++ar->last_wlen;
    break;
    case CMD_END_1:;
      if(ar->ending.length == 1 && !strcmp(ar->ending.str, "&"))
         empty_buffer(&ar->ending),ar->in_background = true;
    case CMD_END_2:;
      ar->extra_read = 1 + ar->f.cur_st - CMD_END_1;
      finish_empty_word_simplecmd(ar);
      ar->finished_reading = true;
      ar->f.cur_st = DEFAULT;
    break;
    case ARGS_INVALID:
      fprintf(stderr, "error parsing arguments\n");
      exit(1);
    break;
    default:;
      assert(ar->f.cur_st < NO_ARGS_STATES);
    break;
  }
  pr_log("STREAMS: ");print_buffer(ar->streams);
  pr_log("ARGMNTS: ");print_buffer(ar->argvbuf);
  pr_log("ENDING: ");print_buffer(ar->ending);
  pr_log("sep_args==%s\nsep_streams==%s\n",
         ar->should_separate_args?"true":"false",
         ar->should_separate_streams?"true":"false");
}

// update command operation reader with a character
void read_command_linker(cmdlinker_reader *cr, char c) {
  process_char(&cr->f, c);
  addch_buffer(&cr->buf, c);
  pr_log("CMDLINKER_BUF: ");print_buffer(cr->buf);
  switch(cr->f.cur_st) {
    case CMDOP_VERT:
      cr->operation = CMD_PIPE;
    break;
    case CMDOP_VERT2:
      cr->operation = CMD_OR;
    break;
    case CMDOP_AMPERS2:
      cr->operation = CMD_AND;
    break;
    case CMDOP_NOOP:
      cr->operation = CMD_NOOP;
    break;
    case CMDOP_INVALID:
      fprintf(stderr, "error parsing intercommand operation\n");
      exit(1);
    break;
    case CMDOP_END_0:
    case CMDOP_END_1:
      cr->extra_read = cr->f.cur_st - CMDOP_END_0;
      cr->finished_reading = true;
    break;
  }
}

// get the next reader using a reader
CMDLINKER parse_cmdlinker(filestream *infile, int *len) {
init_cmdop_reader:;
  *len = 0; char c;
  cmdlinker_reader cr = {
    .f = make_cmdop_fsa(),
    .buf = new_buffer(8),
    .operation = CMD_NOOP,
    .extra_read = 0,
    .finished_reading = false,
  };
reading_loop:;
  while(1) {
    c = read_character(infile),++*len;
    if(c == EOF)infile->seen_eof=true;
    pr_log("reading char %d\n", c==EOF?'\n':c);
    read_command_linker(&cr, c==EOF?'\n':c);
    if(cr.finished_reading) {
      if(c==EOF)rev_character(infile,c);
      break;
    }
    if(c == EOF) {
      fprintf(stderr, "error: parsing input (cmdlinker)\n");
      exit(1);
    }
  }
fix_extra_characters:;
  for(int i=0;i<cr.extra_read;++i)rev_character(infile,pop_ch(&cr.buf));
  assert(cr.operation != CMD_INVALID);
  CMDLINKER op = cr.operation;
cleanup_cmdop_reader:
  clear_fsa(cr.f);
  free_buffer(cr.buf);
  return op;
}

// update stream reader by a character
void read_stream(stream_reader *sr, char c) {
    assert(c != BUF_SEP && c != '\0');
    char printable_c = (c==PIPE_LEFT)?'<':(c==PIPE_RIGHT)?'>':c;
    process_char(&sr->f, printable_c);
    switch(sr->f.cur_st) {
      case STREAM_INPUT_TYPE:
        switch(printable_c) {
          case '1': sr->midstream = STDOUT; break;
          case '2': sr->midstream = STDERR; break;
          case '&': sr->midstream = STDOUT|STDERR; break;
          default: abort(); break;
        }
      break;
      case INPUT_ACTION:
        sr->action = INPUT;
      break;
      case INPUT_INLINE_ACTION:
        sr->action = INPUT_INLINE;
      break;
      case OUTPUT_WRITE_ACTION:
        if(!sr->midstream)
          sr->midstream = STDOUT;
        sr->action = OUTPUT_W;
      break;
      case OUTPUT_APPEND_ACTION:
        if(!sr->midstream)
          sr->midstream = STDOUT;
        sr->action = OUTPUT_A;
      break;
      case STREAM_OUTPUT_TYPE:
        sr->with_filename = false;
        switch(c) {
          case '1': sr->outstream = STDOUT; break;
          case '2': sr->outstream = STDERR; break;
          default: exit(1);
        }
      break;
      case STREAM_FILENAME:
        addch_buffer(sr->filename, printable_c);
        sr->with_filename=true;
      break;
      case STREAM_INVALID:
        fprintf(stderr, "error parsing streams");
        exit(1);
      break;
    }
}

// parse simplecmd stream buffer into streaming options using stream_reader
void parse_streams(simplecmd *s, const buffer_t streams) {
  buffer_t filename = new_buffer(1026);
  for(const char *c = streams.str; c - streams.str < streams.length; ++c) {
    stream_reader sr = {
      .f = make_stream_fsa(),
      .s = s,
      .filename = &filename,
      .instream = STDIN,
      .midstream = 0x00,
      .outstream = STDOUT,
      .action = 0x00,
      .with_filename = false,
    };
    while(*c != BUF_SEP && *c != '\0') {
      read_stream(&sr, *c);
      ++c;
    }
    pr_log("stream input type %d\n", sr.instream);
    pr_log("stream middle type %d\n", sr.midstream);
    pr_log("stream output type %d\n", sr.outstream);
    pr_log("stream action %d\n", sr.action);
    pr_log("relevant filename: "); print_buffer(filename);
    pr_log("with file: %s\n", sr.with_filename?"true":"false");
    void *streams[3] = {[0]=stdin,[1]=stdout,[2]=stderr};
    if(!sr.with_filename) {
      assert(sr.action == OUTPUT_W || sr.action == OUTPUT_A);
      assert(sr.outstream == STDOUT || sr.outstream == STDERR);
      if(sr.midstream & STDOUT)
        assign_stream(&s->output_fname, streams[sr.outstream]);
      if(sr.midstream & STDERR)
        assign_stream(&s->error_fname, streams[sr.outstream]);
    } else {
      if(sr.action == INPUT || sr.action == INPUT_INLINE) {
        assign_stream(&s->input_fname, filename.str);
      } else if(sr.action == OUTPUT_W || sr.action == OUTPUT_A) {
        FD_FLAGS fdflag;
        if(sr.action == OUTPUT_W)
          fdflag = FLAG_OUTPUT_WR;
        if(sr.action == OUTPUT_A)
          fdflag = FLAG_OUTPUT_APP;
        if(sr.midstream & STDOUT)
          assign_stream(&s->output_fname, filename.str);
          s->stdout_flag = fdflag;
        if(sr.midstream & STDERR)
          assign_stream(&s->error_fname, filename.str);
          s->stderr_flag = fdflag;
      }
    }
    empty_buffer(&filename);
    clear_fsa(sr.f);
  }
  free_buffer(filename);
}

// parse simplecmd using simplecmd_reader
simplecmd parse_simplecmd(filestream *infile, int *len) {
  pr_log("PARSING COMMAND\n");
create_argreader:;
  simplecmd_reader ar = {
    .f = make_args_fsa(),
    .argc = 0,
    .argvbuf = new_buffer(1026),
    .streams = new_buffer(1026),
    .ending = new_buffer(8),
    .last_wlen = 0,
    .last_state = DEFAULT,
    .last_char = EOF,
    .reading_mode = 0,
    .should_separate_streams = false,
    .should_separate_args = false,
    .first_space = true,
    .repeated_space = false,
    .in_background = false,
    .finished_reading = false,
    .extra_read = 0,
  };
loop_over_input:;
  *len = 0; char c;
  while(1) {
    c = read_character(infile),++*len;
    if(c==EOF)infile->seen_eof=true;
    pr_log("reading char %d\n", c);
    read_simplecmd_character(&ar, c==EOF?'\n':c);
    if(ar.finished_reading)
      break;
    if(c == EOF) {
      fprintf(stderr, "error: parsing input\n");
      exit(1);
    }
    ar.last_char = c;
  }
check_final_state:;
  if(ar.f.cur_st != DEFAULT) {
    fprintf(stderr, "error: parsing input\n");
    exit(1);
  }
fix_extra_input:;
  *len -= ar.extra_read;
  for(int i=0;i<ar.extra_read;++i)
    rev_character(infile,!i?c:pop_ch(&ar.argvbuf));
make_simplecmd:;
  pr_log("BUFFERS\n");
  pr_log("STREAMS: "),print_buffer(ar.streams);
  pr_log("ARGMNTS: "),print_buffer(ar.argvbuf);
  int cmdl_len;
  pr_log("READ %d CHARACTERS FOR CMD\n", *len);
  CMDLINKER oper = parse_cmdlinker(infile, &cmdl_len);
  pr_log("READ %d CHARACTERS FOR CMDL_LEN\n", cmdl_len);
  simplecmd s = new_simplecmd(ar.argvbuf, ar.argc + 1);
  s.next.operation = oper;
  parse_streams(&s, ar.streams);
  /* dump_simplecmd(s); */
cleanup_argreader:;
  free_buffer(ar.streams);
  free_buffer(ar.argvbuf);
  free_buffer(ar.ending);
  clear_fsa(ar.f);
  return s;
}
