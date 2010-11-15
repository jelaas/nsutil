/*
 * File: macvlan.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

/* ip link add link eth0 address 00:19:d2:49:d2:56 macvlan0 type macvlan */

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
#include "lxc_macvlan_create.h"

#include "jelist.h"
#include "jelopt.h"

#ifndef MACVLAN_MODE_PRIVATE
#  define MACVLAN_MODE_PRIVATE 1 /* don't talk to other macvlans */
#endif

#ifndef MACVLAN_MODE_VEPA
#  define MACVLAN_MODE_VEPA 2 /* talk to other ports through ext bridge */
#endif

#ifndef MACVLAN_MODE_BRIDGE
#  define MACVLAN_MODE_BRIDGE 4 /* talk to bridge ports directly */
#endif

struct args {
  int argc;
  char **argv;
};

struct netdev {
  int ifindex;
  const char *a, *b;
	int mode;
};

struct netdev *netdev_new(const char *a, const char *b, int mode)
{
  struct netdev *d;
  d = malloc(sizeof(struct netdev));
  if(d) {
    d->a = a;
    d->b = b;
    d->mode = mode;
  }
  return d;
}

int main(int argc, char **argv)
{
  struct jlhead *devs;
  struct netdev *netdev;
  int rc=0;
  int err=0;
  char *values[3];
    
  devs = jl_new();

  if(jelopt(argv, 'h', "help", NULL, NULL)) {
	  printf("macvlan -c --create MASTER NAME (private|vepa|bridge)\n"
		 " Create a macvlan interface.\n"
		  );
	  exit(0);
  }


  /* export named if into namespace and rename them */
  while(jelopt_multi(argv, 'c', "create",
		     3, values,
		     &err)) {
	  int mode=0;
	  if(!strcmp(values[2], "private")) mode = MACVLAN_MODE_PRIVATE;
	  if(!strcmp(values[2], "vepa")) mode = MACVLAN_MODE_VEPA;
	  if(!strcmp(values[2], "bridge")) mode = MACVLAN_MODE_BRIDGE;
	  if(mode==0) {
		  rc = 1;
		  fprintf(stderr,
			  "ERROR: mode is either private, vepa or bridge\n");
	  } else
		  jl_append(devs, netdev_new(values[0], values[1], mode));  
	}

  if(devs->len == 0) {
    rc = 1;
    fprintf(stderr, "No interface names to create\n");
  }

  if(rc) {
    fprintf(stderr, "Bailing out.\n");
    exit(rc);
  }
  
  jl_foreach(devs, netdev) {
	  if(lxc_macvlan_create(netdev->a, netdev->b, netdev->mode))
		  fprintf(stderr,
			  "Failed to create macvlan interface %s->%s\n", 
			  netdev->a, netdev->b);
	  else
		  fprintf(stderr, "%s %s\n", netdev->a, netdev->b);
  }
  exit(0);
}
