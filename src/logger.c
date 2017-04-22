#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "logger.h"
#include "filesys.h"

#ifndef NDEBUG
FILE *logfile = NULL;
#endif

// open logfile
void start_log(const char *filename) {
#ifndef NDEBUG
  logfile = fopen(filename, "w");
  assert(logfile != NULL);
#endif
}

// printf for the logger.
// called so to match the length of printf command
void pr_log(const char *fmt, ...)  {
#ifndef NDEBUG
  if(logfile == NULL)
    logfile = stderr;
  va_list argptr;
  va_start(argptr, fmt);
  vfprintf(logfile, fmt, argptr);
  va_end(argptr);
  fflush(logfile);
#endif
}

// chek if file is stdin/stdout/stderr
static bool isstream(FILE *file) {
  return !isfile(file);
}

// close logfile
void close_log() {
#ifndef NDEBUG
  if(!(logfile == NULL || isstream(logfile)))
    fclose(logfile);
#endif
}
