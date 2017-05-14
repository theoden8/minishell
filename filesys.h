#ifndef FILESYS_H_CCJIZRFD
#define FILESYS_H_CCJIZRFD

#include <stdbool.h>

/*
 * filesys contains some functions which ease the access to the file system
 */

typedef enum {
  FLAG_INPUT, FLAG_OUTPUT_WR, FLAG_OUTPUT_APP
} FD_FLAGS;

bool isfile(const void *ptr);
int fd_is_valid(int fd);
int open_file(const char *filename, FD_FLAGS flag);
int dup_stream(int fd1, int fd2);
void pipe_streams(int out1,int in2);
void close_file(int fd);

int set_stdin(const void *filename);
int set_stdout(const void *filename, FD_FLAGS flag);
int set_stderr(const void *filename, FD_FLAGS flag);

#endif
