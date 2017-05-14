#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>

#include "filesys.h"
#include "multiproc.h"
#include "logger.h"
#include "random.h"

// creates a semaphore and deals with errors
sem_t *create_semaphore(const char *semname, int value) {
  pr_log("Creating semaphore '%s'\n", semname);
  sem_t *semaphore = sem_open(semname, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, value);
  if(semaphore == SEM_FAILED) {
    perror("sem_open(3)");
    exit(EXIT_FAILURE);
  }
  return semaphore;
}

// opens an existing semaphore and deals with errors
sem_t *open_semaphore(const char *semname) {
  sem_t *semaphore = sem_open(semname, O_RDWR);
  if(semaphore == SEM_FAILED) {
    perror("sem_open(3)");
    exit(EXIT_FAILURE);
  }
  return semaphore;
}

// closes an opened semaphore and deals with errors
void close_semaphore(sem_t *sem, const char *semname) {
  if(sem_close(sem) < 0) {
    perror("sem_close(3)");
    /* We ignore possible sem_unlink(3) errors here */
    delete_semaphore(semname);
    exit(EXIT_FAILURE);
  }
}

// locks a semaphore
void lock_semaphore(sem_t *sem) {
  if(sem_wait(sem) < 0) {
    perror("sem_wait(3)");
  }
}

// unlocks a semaphore
void unlock_semaphore(sem_t *sem) {
  if(sem_post(sem) < 0) {
    perror("sem_post(3)");
  }
}

// deletes a semaphore
void delete_semaphore(const char *semname) {
  pr_log("Deleting semaphore '%s'\n", semname);
  if(sem_unlink(semname) < 0) {
    perror("sem_unlink(3)");
  }
}

// give a location of the temporary output storage for a given execution index
// important: needs to be run before forking so all files get the same infix
const char *get_temp_filename(int fork_id) {
  static bool first_time = true;
  static int RANDOM;
  if(first_time) {
    RANDOM = rand();
    first_time = false;
  }
  static char buf[128];
  sprintf(buf, "temp_%d%d", RANDOM, fork_id);
  return buf;
}

// create temporary file and modify simplecmd
void create_temp(simplecmd *s) {
  assert(s->fork_id >= 0);
  s->fd_fork = open_file(get_temp_filename(s->fork_id), FLAG_OUTPUT_WR);
}

// close the temporary file
void close_temp(simplecmd *s) {
  assert(s->fork_id >= 0);
  close_file(s->fd_fork);
  s->fd_fork = -1;
}

// prints and deletes a temporary file
void read_temp(int fork_id) {
  assert(access(get_temp_filename(fork_id), F_OK) != -1);
  FILE *fpart = fopen(get_temp_filename(fork_id), "r");
  assert(fpart != NULL);
  char c;
  while((c = fgetc(fpart)) != EOF)putchar(c);
  assert(fpart != NULL);
  fclose(fpart);
  remove(get_temp_filename(fork_id));
}
