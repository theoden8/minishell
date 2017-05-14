#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "logger.h"
#include "buffer.h"
#include "parser.h"

main() {
  start_log("log/runcmds.log");
  int len;
  filestream in = make_filestream(stdin);
  while(!in.seen_eof) {
    simplecmd s = parse_simplecmd(&in, &len);
    assert(!errno);
    execute_simplecmd(&s);
    free_simplecmd(s);
  }
  unmake_filestream(in);
  close_log();
}
