#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "random.h"

// set seed using /dev/random
void seed_rng() {
  int fd=open("/dev/urandom", O_RDONLY);
  if(fd==-1) abort();
  unsigned seed,pos = 0;
  while(pos<sizeof(seed)){int a=read(fd,(char*)&seed+pos,sizeof(seed)-pos);if(a<=0)abort();pos += a;}
  srand(seed);
  close(fd);
}
