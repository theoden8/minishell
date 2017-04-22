#ifndef FILESTREAM_H_UF6SF1KE
#define FILESTREAM_H_UF6SF1KE

#include <stdio.h>

#include "buffer.h"

/*
 * filestream is an impleemntation of a reversable file.
 * ungetc guarantees to work only once, whereas this guarantees
 * as many characters reverse as it might be needed.
 */


typedef struct {
  FILE *in;
  buffer_t rev;
  bool seen_eof : 1;
} filestream;

filestream make_filestream(FILE *file);
char read_character(filestream *infile);
void rev_character(filestream *infile, char c);
void unmake_filestream(filestream fstrm);

#endif
