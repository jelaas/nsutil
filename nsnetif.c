/*
 * File: nsnetif.c
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
#include "lxc_device_rename.h"

#include "jelist.h"
#include "jelopt.h"

struct args {
  int argc;
  char **argv;
};

struct netdev {
  int ifindex;
  const char *ifname, *newname;
};

struct netdev *netdev_new(int ifindex, const char *ifname, const char *newname)
{
  struct netdev *d;
  d = malloc(sizeof(struct netdev));
  if(d) {
    d->ifindex = ifindex;
    d->ifname = ifname;
    d->newname = newname;
  }
  return d;
}

int main(int argc, char **argv)
{
  pid_t pid = 0;
  struct jlhead *devs;
  int ifindex = 100;
  struct netdev *netdev;
  int rc=0;
  char *ifname = NULL;
  int err=0;
  char *values[2];
    
  devs = jl_new();

  if(jelopt(argv, 'h', "help", NULL, NULL)) {
	  printf("nsnetif [-p --pid NSPID] (-i IF|-n IFOLD IFNEW)\n"
		 " Move network device(s) into a namespace.\n"
		  );
	  exit(0);
  }

  /* export named if into namespace */
  while(jelopt(argv, 'i', "ifname",
	       &ifname, &err)) {
    /* extract any devices we should move into the namespace from argv */
    ifindex = if_nametoindex(ifname);
    if(ifindex <= 1) {
      fprintf(stderr, "Cannot find device %s\n", ifname);
      rc = 1;
    }
    jl_append(devs, netdev_new(ifindex, ifname, NULL));
  }

  /* rename the named if, then export if into namespace */
  while(jelopt_multi(argv, 'n', "ifrename", 2, values,
		     &err)) {
    ifname = values[0];
    ifindex = if_nametoindex(ifname);
    if(ifindex <= 1) {
      fprintf(stderr, "Cannot find device %s\n", ifname);
      rc = 1;
    }
    jl_append(devs, netdev_new(ifindex, ifname, values[1]));	  
  }
	
	
  if(jelopt_int(argv, 'p', "pid",
		&pid, &err))
    ;

  if(pid == 0) {
    char *pidstr;
    pidstr = getenv("NSPID");
    if(pidstr) {
      pid = atoi(pidstr);
    }
  }

  if(pid == 0) {
    fprintf(stderr, "You need to give --pid parameter\n");
    rc = 1;
  }

  if(rc) {
    fprintf(stderr, "Bailing out.\n");
    exit(rc);
  }
  
  jl_foreach(devs, netdev) {
    if(netdev->newname) {
      if((rc=lxc_device_rename(netdev->ifname, netdev->newname))) {
	fprintf(stderr, "Failed to rename %d to %s [%d]\n",
		netdev->ifindex,
		netdev->newname,
		rc);
	rc=1;
      }
    }
  }

  /* move given network devices into namespace */
  jl_foreach(devs, netdev) {
    if((rc=lxc_device_move(netdev->ifindex, pid))) {
      if(rc == -1) fprintf(stderr, "Failed to open netlink.\n");
      if(rc == -4) fprintf(stderr, "Netlink transaction failed.\n");
      fprintf(stderr, "Failed to move %d into namespace [%d]\n", netdev->ifindex, rc);
      rc=1;
    }
  }

  exit(rc);
}
