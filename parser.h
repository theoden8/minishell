#ifndef PARSER_H_SUJEC4YA
#define PARSER_H_SUJEC4YA

#include <stdbool.h>

#include "fsa.h"
#include "buffer.h"
#include "executor.h"
#include "filestream.h"

/*
 * parsing a subset of shell grammar
 */

// a structure for reading simplecmd (from executer)
typedef struct {
  fsa_t f; // argument parsing fsa
  int argc : 12; // number of arguments
  buffer_t argvbuf; // buffer to push argument characters to
  buffer_t streams; // buffer to push stream characters to
  buffer_t ending; // necessary for dealing with the context of & (one of: &>.., ..>&., ... &, .. && ..)
  int last_wlen : 12; // length of the last word
  int last_state : 8;
  char last_char;
  bool
    reading_mode : 1, // 0 for argvbuf, 1 for streambuf
    should_separate_streams : 1, // next relevant character will split streams
    should_separate_args : 1, // next relevant character will split argvbuf
    first_space : 1, // if it is the first space before any arguments/streams
    repeated_space : 1, // whether the current whitespace is repeating. could use WHITESPACE_BEGIN, but it's longer
    in_background : 1, // unused, indicates whether the command being parsed will or not be executed in background
    finished_reading : 1; // whether to continue parsing or not
  int extra_read : 3; // how many characters to push back to filestream
} simplecmd_reader;

// a few enums for convenience of parsing streams
typedef enum { STDIN = 0, STDOUT = 1, STDERR = 2, CHANGED } stream_type;
typedef enum { INPUT = 1, INPUT_INLINE, OUTPUT_W, OUTPUT_A } stream_action;

// a structure for reading streams
typedef struct {
  fsa_t f; // stream parsing fsa
  simplecmd *s;
  buffer_t *filename; // buffer for storing filename
  stream_type
    instream, // <_
    midstream, // _>
    outstream; // >_
  stream_action action; // what to do with the streams
  bool with_filename : 1; // the stream uses filename
} stream_reader;

// a structure for reading intercommand operations and separators between
// commands
typedef struct {
  fsa_t f; // command operation parsing fsa
  buffer_t buf; // currently read characters
  CMDLINKER operation; // operation found
  int extra_read : 3; // how many characters to put back to filestream
  bool finished_reading : 1; // whether to continue reading or not
} cmdlinker_reader;

buffer_t *get_cur_buf(simplecmd_reader *ar);
void finish_empty_word_simplecmd(simplecmd_reader *ar);
void split_words_simplecmd(simplecmd_reader *ar);
void read_simplecmd_character(simplecmd_reader *ar, const char c);
void read_command_linker(cmdlinker_reader *cr, char c);
CMDLINKER parse_cmdlinker(filestream *infile, int *len);
void read_stream(stream_reader *sr, char c);
void parse_streams(simplecmd *s, const buffer_t streams);
simplecmd parse_simplecmd(filestream *infile, int *len);

#endif
