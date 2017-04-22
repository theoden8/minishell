#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/sem.h>

#include "parser.h"
#include "multiproc.h"
#include "buffer.h"
#include "logger.h"
#include "random.h"

main(int argc, char *argv[]) {
  start_log("runparallelcmds.log");
  int numprocs = 2;
  // getopt
  int choice;
  while(1) {
    static struct option long_options[] = {
      {"-j", required_argument, NULL, 'j'},
      {NULL,0,NULL,'\0'}
    };
    int option_index = 0;
    choice = getopt_long(argc, argv, "j:", long_options, &option_index);
    if(choice == -1)
      break;
    switch(choice){
      case'j':numprocs=atoi(optarg),numprocs=(numprocs>0)?numprocs:1;break;
      case'?':break;
      default:return EXIT_FAILURE;
    }
  }
  int len;
  filestream in = make_filestream(stdin);
  // we need to run it once to initialize static variables
  // the argument doesn't matter, as we don't care about the return value
  seed_rng();
  get_temp_filename(len);
  char SEMAPHORE_NAME[80];
  sprintf(SEMAPHORE_NAME, "/sem/w11prac_%d", rand());
  int fork_id = 0;
  sem_t *semaphore = create_semaphore(SEMAPHORE_NAME, numprocs);
  close_semaphore(semaphore, SEMAPHORE_NAME);
  while(!in.seen_eof) {
    simplecmd s = parse_simplecmd(&in, &len);
    int child = fork();
    if(child != 0) {
      ++fork_id;
      free_simplecmd(s);
    } else {
      s.fork_id = fork_id;
      open_semaphore(SEMAPHORE_NAME);
      int fd = open(get_temp_filename(fork_id), O_CREAT | O_TRUNC, 0666);close(fd);
      lock_semaphore(semaphore);
      execute_simplecmd(&s);
      unlock_semaphore(semaphore);
      close_semaphore(semaphore, SEMAPHORE_NAME);
      free_simplecmd(s);
      unmake_filestream(in);
      close_log();
      return 0;
    }
  }
  while(waitpid(-1,NULL,0)!=-1)
    ;
  delete_semaphore(SEMAPHORE_NAME);
  for(int f = 0; f<fork_id; ++f)
    read_temp(f);
  unmake_filestream(in);
  close_log();
}
