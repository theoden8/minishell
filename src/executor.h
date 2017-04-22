#ifndef EXECUTOR_H_SWN1YIOT
#define EXECUTOR_H_SWN1YIOT

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "filesys.h"
#include "buffer.h"

/*
 * executes parsed parts of the program
 */

struct _simplecmd;

typedef enum { CMD_NOOP, CMD_PIPE, CMD_AND, CMD_OR, CMD_INVALID } CMDLINKER;

typedef struct _cmd_linker {
  CMDLINKER operation;
  struct _simplecmd *cmd;
} cmd_linker;

typedef struct _simplecmd {
  int fd_stdin, fd_stdout, fd_stderr;
  FD_FLAGS stdout_flag, stderr_flag;
  char **argv;
  int argc;
  int ret : 8;
  int fork_id;
  int fd_fork;
  bool in_background : 1;
  void *input_fname;
  void *output_fname;
  void *error_fname;
  struct _cmd_linker next;
} simplecmd;

simplecmd new_simplecmd(buffer_t buf, int argc);
void dump_simplecmd(simplecmd s);
void free_simplecmd(simplecmd s);

void assign_stream(void **stream, void *assigned);

void process_simplecmd_streams(simplecmd *s, const buffer_t streams);
void set_streams(simplecmd *s);
void dup_streams(simplecmd *s);
void execution_setup_simplecmd(simplecmd *s);
void execution_finish_simplecmd(simplecmd *s);
void execute_with_result(simplecmd *s);
void execute_simplecmd(simplecmd *s);

#endif
