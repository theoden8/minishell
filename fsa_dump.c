#include <stdio.h>

#include "fsa.h"
#include "logger.h"
#include "random.h"

main() {
  start_log("log/fsa_dump.log");
  seed_rng();
  puts("CMDOP DUMP");
  fsa_spit_sample_cmdop(),putchar('\n');
  puts("STREAM DUMP");
  fsa_spit_sample_stream(),putchar('\n');
  puts("ARGS DUMP");
  fsa_spit_sample_args(),putchar('\n');
  close_log();
}
