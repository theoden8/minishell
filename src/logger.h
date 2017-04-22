#ifndef LOGGER_H_6IY1JHTR
#define LOGGER_H_6IY1JHTR

/*
 * implementation of logging. to be initialized at start and closed in the end.
 * solely for debugging purposes, -DNDEBUG removes it
 */

extern FILE *logfile;

void start_log(const char *filename);
void pr_log(const char *fmt, ...);
void close_log();

#endif
