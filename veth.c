/*
 * File: veth.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */


#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <alloca.h>
#include <stdio.h>
#include <net/if.h>
#include <string.h>

#include "lxc_device_move.h"
#include "lxc_veth_create.h"

#include "jelist.h"
#include "jelopt.h"

struct args {
  int argc;
  char **argv;
};

struct netdev {
  int ifindex;
  const char *a, *b;
};

struct netdev *netdev_new(const char *a, const char *b)
{
  struct netdev *d;
  d = malloc(sizeof(struct netdev));
  if(d) {
    d->a = a;
    d->b = b;
  }
  return d;
}

int main(int argc, char **argv)
{
  struct jlhead *devs;
  struct netdev *netdev;
  int rc=0;
  int err=0;
  char *values[2];
    
  devs = jl_new();

  if(jelopt(argv, 'h', "help", NULL, NULL)) {
	  printf("veth -c --create IF1 IF2\n"
		 " Create a pair of veth interfaces.\n"
		  );
	  exit(0);
  }


  /* export named if into namespace and rename them */
  while(jelopt_multi(argv, 'c', "create",
		     2, values,
		     &err)) {
	  jl_append(devs, netdev_new(values[0], values[1]));  
	}

  if(devs->len == 0) {
    rc = 1;
    fprintf(stderr, "No interface names to create\n");
  }

  if(rc) {
    fprintf(stderr, "Bailing out.\n");
    exit(rc);
  }
  
  /* move given network devices into namespace */
  jl_foreach(devs, netdev) {
    if(lxc_veth_create(netdev->a, netdev->b))
      fprintf(stderr, "Failed to create veth pair %s-%s\n", 
	      netdev->a, netdev->b);
    else
      fprintf(stderr, "%s %s\n", netdev->a, netdev->b);
  }
  exit(0);
}
