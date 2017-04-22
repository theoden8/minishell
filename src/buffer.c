#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "logger.h"

// strdup from string.h, which is not always found in the system
char *strdup(const char *s) {
  size_t len = strlen(s) + 1;
  char *s2 = malloc(sizeof(char) * len);
  assert(s2 != NULL);
  memcpy(s2, s, len * sizeof(char));
  return s2;
}

// check if the symbol is a reserved pipe indicator
bool is_pipe_sym(char c) {
  return c == PIPE_LEFT || c == PIPE_RIGHT;
}

// allocate a new buffer of certain size
buffer_t new_buffer(size_t max) {
  buffer_t b = {
    .str = malloc(sizeof(int) * max),
    .length = 0,
    .maxlen = max,
  };
  assert(b.str != NULL);
  b.str[0] = '\0';
  return b;
}

// push a character to the end of the buffer string
void addch_buffer(buffer_t *buf, char c) {
  assert(buf->length <= buf->maxlen);
  buf->str[buf->length]=c;
  buf->str[++buf->length]='\0';
}

// push a separator to the end of the buffer
void split_buffer(buffer_t *buf) {
  addch_buffer(buf, BUF_SEP);
}

// get the last character before the terminating 0
char last_ch(buffer_t buf) {
  if(buf.length == 0)
    return EOF;
  return buf.str[buf.length - 1];
}

// pop the last character from the buffer (and shift the terminating 0)
char pop_ch(buffer_t *buf) {
  if(buf->length == 0)
    return EOF;
  char c = last_ch(*buf);
  buf->str[--buf->length] = '\0';
  return c;
}

// make the string ""
void empty_buffer(buffer_t *buf) {
  buf->length=0;
  buf->str[buf->length]='\0';
}

// print the string in the buffer with respect to special constants
void print_buffer(buffer_t buf) {
  for(size_t i=0;i<buf.length;++i) {
    char c = buf.str[i];
    switch(c) {
      case BUF_SEP:pr_log(", ");break;
      case PIPE_LEFT:pr_log("[\\<]");break;
      case PIPE_RIGHT:pr_log("[\\>]");break;
      default:pr_log("%c", c);break;
    }
  }
  pr_log("\n");
  fflush(stdout);
}

// deallocate the data in buffer
void free_buffer(buffer_t buf) {
  free(buf.str);
}
