#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "filesys.h"
#include "logger.h"

/*
 * Because checks if simplecmd fname pointer points to a filename or to a
 * stream.
 */
bool isfile(const void *ptr) {
  return !(ptr == stdin || ptr == stdout || ptr == stderr);
}

/*
 * checks if a file descriptor is used.
 */
int fd_is_valid(fd) {
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

/*
 * opens a file for one of three options:
 *    reading
 *    writing
 *    appending
 */
int open_file(const char *filename, FD_FLAGS flag) {
  pr_log("flag==%d\n", flag);
  errno=0;
  int fdflag;
  switch(flag) {
    case FLAG_INPUT:
      fdflag = O_RDONLY;
    break;
    case FLAG_OUTPUT_WR:
      fdflag = O_CREAT | O_WRONLY | O_TRUNC;
    break;
    case FLAG_OUTPUT_APP:
      fdflag = O_CREAT | O_APPEND;
    break;
  }
  int fd = open(filename, fdflag, 0666);
  #ifndef STACSCHECK
  if(errno){
    char buf[256];
    sprintf(buf, "error opening file \"%s\"", filename);
    perror(buf);
    errno=0;
  }
  #endif
  return fd;
}

/*
 * dup with debugging
 */
int dup_stream(fd1,fd2) {
  int dupe = dup2(fd1, fd2);
  assert(dupe != -1);
  if(errno) {
    #ifndef STACSCHECK
    char buf[80];
    sprintf(buf, "error (dup [%d, %d])", fd1, fd2);
    perror(buf);
    #endif
    errno = 0;
  }
  return dupe;
}

// little shortcut with possibility to add some debugging
void pipe_streams(out1,in2) {
  int fd[2]={out1,in2};
  pipe(fd);
}

/*
 * close and check for errors
 */
void close_file(int fd) {
  close(fd);
  #ifndef STACSCHECK
  if(errno) {
    char buf[256];
    sprintf(buf, "error closing file \"%d\"", fd);
    perror(buf);
    errno = 0;
  }
  #endif
}

// get input file/stream descriptor
int set_stdin(const void *filename) {
  if(isfile(filename))
    return open_file(filename, FLAG_INPUT);
  if(filename == stdin)
    return dup(STDIN_FILENO);
  fprintf(stderr, "error: couldn't set stdin");
  abort();
}

// get output file/stream descriptor
int set_stdout(const void *filename, FD_FLAGS flag) {
  if(isfile(filename))
    return open_file(filename, flag);
  if(filename == stdout)
    return dup(STDOUT_FILENO);
  if(filename == stderr)
    return dup(STDERR_FILENO);
  fprintf(stderr, "error: couldn't set stdin (%p)", filename);
  abort();
}

// get error file/stream editor
int set_stderr(const void *filename, FD_FLAGS flag) {
  if(isfile(filename))
    return open_file(filename, flag);
  if(filename == stdout)
    return dup(STDOUT_FILENO);
  if(filename == stderr)
    return dup(STDERR_FILENO);
  fprintf(stderr, "error: couldn't set stdin (%p)", filename);
  abort();
}
