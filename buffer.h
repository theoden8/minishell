#ifndef BUFFER_H_NAOVFWMC
#define BUFFER_H_NAOVFWMC

#include <stdbool.h>

#define BUF_SEP (-1)
#define PIPE_LEFT (-2)
#define PIPE_RIGHT (-3)

/*
 * buffer is user for storing strings with a limited max size, always terminated
 * with zero
 */

extern const size_t BUF_LENGTH;

char *strdup(const char *s);

bool is_pipe_sym(char c);

typedef struct {
  char *str;
  size_t length;
  size_t maxlen;
} buffer_t;

buffer_t new_buffer(size_t max);
void add_buffer(buffer_t *src, buffer_t *dst);
void addch_buffer(buffer_t *buf, char c);
void split_buffer(buffer_t *buf);
char last_ch(buffer_t buf);
char pop_ch(buffer_t *buf);
void empty_buffer(buffer_t *buf);
void print_buffer(buffer_t buf);
void free_buffer(buffer_t buf);

#endif
