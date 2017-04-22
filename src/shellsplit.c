#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include "buffer.h"
#include "fsa.h"
#include "parser.h"
#include "executor.h"
#include "logger.h"
#include "random.h"

void explain_simplecmd(simplecmd s) {
  printf("Run program \"%s\"", s.argv[0]);
  if(s.argc > 1) {
    printf(" with %s ", (s.argc == 2) ? "argument" : "arguments");
    int i;
    for(i=1;i<s.argc-1;++i)
      printf("\"%s\" and ", s.argv[i]);
    printf("\"%s\"", s.argv[i]);
  }
  printf(".");
  if(isfile(s.input_fname))
    printf(" Read the input from file \"%s\".", s.input_fname);
  if(isfile(s.output_fname))
    printf(" Write the output to file \"%s\".", s.output_fname);
  else if(s.output_fname == stderr)
    printf(" Write the output to <stderr>.");
  printf("\n");
}

main(int argc, char *argv[]) {
  int len;
  start_log("shellsplit.log");
  filestream in = make_filestream(stdin);
  while(!in.seen_eof) {
    simplecmd s = parse_simplecmd(&in, &len);
    explain_simplecmd(s);
    free_simplecmd(s);
  }
  unmake_filestream(in);
  close_log();
}
