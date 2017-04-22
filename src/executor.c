#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "fsa.h"
#include "executor.h"
#include "logger.h"
#include "multiproc.h"

// allocate new simplecmd
simplecmd new_simplecmd(buffer_t buf, int argc) {
  if(!argc) {
    assert(buf.length == 0);
    for(const char *c = "cat"; *c != '\0'; ++c)
      addch_buffer(&buf, *c);
  }
  simplecmd s = {
    .fd_stdin = STDIN_FILENO,
    .fd_stdout = STDOUT_FILENO,
    .fd_stderr = STDERR_FILENO,
    .stdout_flag = FLAG_OUTPUT_WR,
    .stderr_flag = FLAG_OUTPUT_WR,
    .argv = malloc(sizeof(char *) * (argc + 1)),
    .argc = argc,
    .ret = 0,
    .in_background = false,
    .fork_id = -1,
    .fd_fork = -1,
    .input_fname = stdin,
    .output_fname = stdout,
    .error_fname = stderr,
    .next = (cmd_linker){
      .operation = CMD_NOOP,
      .cmd = NULL,
    },
  };
  assert(s.argv != NULL);
  buffer_t argument = new_buffer(1026);
  char *c = buf.str;
  for(int i = 0; i < argc; ++i) {
    while(*c != BUF_SEP && *c != '\0') {
      addch_buffer(&argument, *c);
      ++c;
    }
    s.argv[i] = strdup(argument.str);
    empty_buffer(&argument);
    ++c;
  }
  s.argv[s.argc] = NULL;
  free_buffer(argument);
  return s;
}

// get string representation of a ..._fname field, i.e.
// a const char* or a FILE *
const char *get_filename(const void *filename) {
  if(isfile(filename))return filename;
  if(filename==stdin)return"stdin";
  if(filename==stdout)return"stdout";
  if(filename==stderr)return"stderr";
  return NULL;
}

// debug output of all relevant fields of one simplecmd
void dump_simplecmd(simplecmd s) {
  pr_log("----------------------\n");
  pr_log("SIMPLECOMMAND DUMP\n");
  pr_log("input %d\n", s.fd_stdin);
  pr_log("output %d\n", s.fd_stdout);
  pr_log("error %d\n", s.fd_stderr);
  pr_log("argc == %d\n", s.argc);
  pr_log("argv:\n");
  for(int i = 0; i < s.argc; ++i) {
    pr_log("%d\t'%s'\n", i, s.argv[i]);
  }
  pr_log("input file: %s\n", get_filename(s.input_fname));
  pr_log("output file: %s\n", get_filename(s.output_fname));
  pr_log("error file: %s\n", get_filename(s.error_fname));
  pr_log("in_background==%s\n", s.in_background?"true":"false");
  pr_log("ret==%d\n", s.ret);
  pr_log("next=={%d,%p}\n", s.next.operation, s.next.cmd);
  if(s.next.cmd != NULL)
    dump_simplecmd(*s.next.cmd);
  fflush(stdout);
}

// deallocate simplecmd
void free_simplecmd(simplecmd s) {
  if(s.next.cmd != NULL)
    free_simplecmd(*s.next.cmd);
  for(int i = 0; i < s.argc; ++i)
    free(s.argv[i]);
  if(isfile(s.input_fname))
    free(s.input_fname);
  if(isfile(s.output_fname))
    free(s.output_fname);
  if(isfile(s.error_fname))
    free(s.error_fname);
  free(s.argv);
}

// assign file/stream pointer to the file/stream pointer field
// manages allocation/assignment problems
void assign_stream(void **stream, void *assigned) {
  if(isfile(*stream) && !isfile(assigned))
    free(*stream),*stream=assigned;
  else if(!isfile(*stream) && isfile(assigned))
    *stream = strdup(assigned);
  else
    *stream = assigned;
}

// sets up stdin, stdout and stderr
void set_streams(simplecmd *s) {
  if(s->fork_id >= 0 && !isfile(s->error_fname)) {
    create_temp(s);
    dup_stream(s->fd_fork, STDERR_FILENO);
  }
  int failures = 0;
  s->fd_stdin = set_stdin(s->input_fname);
  if(s->fd_stdin == -1) {
    failures |= 1;
    fprintf(stderr, "Read failed: %s\n", get_filename(s->input_fname));
  }
  s->fd_stdout = set_stdout(s->output_fname, s->stdout_flag);
  if(s->fd_stdout == -1) {
    failures |= 2;
    fprintf(stderr, "Write failed: %s\n", get_filename(s->output_fname));
  }
  s->fd_stderr = set_stderr(s->error_fname, s->stderr_flag);
  if(s->fd_stderr == -1) {
    failures |= 4;
    /* fprintf(stderr, "Error failed: %s\n", get_filename(s->error_fname)); */
  }
  if(failures)s->ret=-1;
}

// dup stdin, stdout, stderr to whatever the redirection is set
void dup_streams(simplecmd *s) {
  int failures = 0;
  int ret;
  assert(fd_is_valid(s->fd_stdin));
  ret=dup_stream(s->fd_stdin, STDIN_FILENO);
  if(ret==-1)failures|=1;
  assert(fd_is_valid(s->fd_stdout));
  ret=dup_stream(s->fd_stdout, STDOUT_FILENO);
  if(ret==-1)failures|=2;
  assert(fd_is_valid(s->fd_stderr));
  ret=dup_stream(s->fd_stderr, STDERR_FILENO);
  if(ret==-1)failures|=4;
  if(failures)s->ret=-1;
  if(s->fork_id >= 0 && !isfile(s->output_fname)) {
    if(s->fd_fork == -1)
      create_temp(s);
    dup_stream(s->fd_fork, STDOUT_FILENO);
  }
}

// unused. a small overhead of using command operations
void pipe_simplecmds(simplecmd *s1, simplecmd *s2) {
  if(s1->input_fname!=stdin)return;
  if(s1->output_fname==stdout)pipe_streams(s1->fd_stdout,s2->fd_stdin);
  if(s1->error_fname==stdout)pipe_streams(s1->fd_stderr,s2->fd_stdin);
}

// opposite to set_stream. closes files, temporaries and similar things
void reset_streams(simplecmd *s) {
  if(s->fork_id >= 0) {
    if(s->fd_fork == -1)
      create_temp(s);
    close_temp(s);
  }
  if(s->fd_stdin != -1)
    close_file(s->fd_stdin),s->fd_stdin = STDIN_FILENO;
  if(s->fd_stdout != -1)
    close_file(s->fd_stdout),s->fd_stdout = STDOUT_FILENO;
  if(s->fd_stderr != -1)
    close_file(s->fd_stderr),s->fd_stderr = STDERR_FILENO;
  if(errno) {
    /* perror("error"); */
    errno=0;
  }
}

// a separated part of the execition process.
// a few checks + setting up streams etc.
void execution_setup_simplecmd(simplecmd *s) {
  if(s == NULL)
    return;
  assert(s->next.operation != CMD_INVALID);
  if(s->in_background && (s->next.cmd != NULL && s->next.operation != CMD_NOOP)) {
    fprintf(stderr, "error: cannot execute lcommand in parallel");
    exit(1);
  }
  pr_log("EXECUTING COMMAND\n");
  assert(!errno);
  set_streams(s);
  dump_simplecmd(*s);
}

// forkable part of the execution.
void execute_with_result(simplecmd *s) {
  if(s == NULL)
    return;
  if(!s->ret) {
    pid_t parent = fork();
    if(errno) {
      /* perror("failed to fork"); */
      errno=0;
    }
    if(parent) {
      int loc;
      pid_t q = wait(&loc);
      #ifdef STACSCHECK
      s->ret = (loc<0)?loc:0;
      #else
      s->ret = WEXITSTATUS(loc);
      #endif
    } else {
      dup_streams(s);
      if(execvp(s->argv[0], s->argv) < 0) {
        #ifdef STACSCHECK
        fprintf(stderr, "Execute failed: %s\n", s->argv[0]);
        free_simplecmd(*s);
        exit(1);
        #endif
      }
      reset_streams(s);
      close_log();
    }
  }
  return;
  // small overhead of using different operations between commands
  // although unused, is important for structuring ideas and keeping code
  // flexible
  switch(s->next.operation) {
    case CMD_PIPE:
      if(s->next.cmd == NULL) {
        fprintf(stderr, "error: no program after pipe\n");
        exit(1);
      }
      execution_setup_simplecmd(s->next.cmd);
      pipe_simplecmds(s, s->next.cmd);
      execute_with_result(s->next.cmd);
      execution_finish_simplecmd(s);
    break;
    case CMD_NOOP:
      execute_simplecmd(s->next.cmd);
    break;
    case CMD_AND:
      if(!s->ret)
        execute_simplecmd(s->next.cmd);
      else if(s->next.cmd != NULL)
        execute_simplecmd(s->next.cmd->next.cmd);
    break;
    case CMD_OR:
      if(!s->ret)
        execute_simplecmd(s->next.cmd);
      else if(s->next.cmd != NULL)
        execute_simplecmd(s->next.cmd->next.cmd);
    break;
  }
}

// last part of the execution. mostly deallocating/closing things whose lifetime
// logically ends after execution.
void execution_finish_simplecmd(simplecmd *s) {
  if(s == NULL)
    return;
  reset_streams(s);
  if(s->ret && (s->next.operation == CMD_NOOP || s->next.operation == CMD_PIPE)) {
    assert(!errno);
    #ifndef STACSCHECK
    fprintf(stderr, "shell returns %d\n", s->ret);
    #endif
  }
  if(errno != 0) {
    perror("error");
    errno = 0;
  }
}

// all parts of the execution together
void execute_simplecmd(simplecmd *s) {
  if(errno) {
    /* perror("before execution"); */
    errno=0;
  }
  execution_setup_simplecmd(s);
  execute_with_result(s);
  execution_finish_simplecmd(s);
}
