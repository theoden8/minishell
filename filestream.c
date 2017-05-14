#include <stdbool.h>

#include "filestream.h"

// set filestream structure, allocate rev buffer
filestream make_filestream(FILE *file) {
  return (filestream){.in=file,.rev=new_buffer(8),.seen_eof=false};
}

// read character from the filestream
char read_character(filestream *infile) {
  if(infile->rev.length)
    return pop_ch(&infile->rev);
  return fgetc(infile->in);
}

// "ungetc", put a character back to the filestream
void rev_character(filestream *infile, char c) {
  addch_buffer(&infile->rev, c);
}

// deallocate filestream rev buffer
void unmake_filestream(filestream fstrm) {
  free_buffer(fstrm.rev);
}
