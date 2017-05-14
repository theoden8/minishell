#ifndef MULTIPROC_H_J7FR8B2Q
#define MULTIPROC_H_J7FR8B2Q

#include <stdio.h>
#include <semaphore.h>

#include "executor.h"

/*
 * helper functions for multiprocessed running
 */

sem_t *create_semaphore(const char *semname, int value);
sem_t *open_semaphore(const char *semname);
void close_semaphore(sem_t *sem, const char *semname);
void lock_semaphore(sem_t *sem);
void unlock_semaphore(sem_t *sem);
void delete_semaphore(const char *semname);

const char *get_temp_filename(int fork_id);
void create_temp(simplecmd *s);
void close_temp(simplecmd *s);
void read_temp(int fork_id);

#endif
