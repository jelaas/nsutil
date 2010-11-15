/*
 * File: nscgdev.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

/*

 import device access to namespace cgroup.

 add access to cgroup for /dev/X

 nscgdev /dev/null -- give access to /dev/null device to NSPID cgroup.
 nscgdev /dev/pty7 -- give access to node for cgroup NSPID.
 to map /dev/pty7 to /dev/console inside the ns we can perform an --init command
 inside NSPID. Note that this is before chroot/pivot_root.

 ns -t fs --hrun "nscgset cpus=0,1" --hrun "nscgdev /dev/$PTY" --nsrun "mknod c $PTYMAJOR $PTYMINOR /dev/console" /sbin/nsrootfs /path/to/rootfs /sbin/init


 - create cgroup if needed. [use NSPID envvar]
 - add device access to cgroup.

 */
#define _BSD_SOURCE
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "jelist.h"
#include "jelopt.h"

#include "cgroup.h"

int main(int argc, char **argv)
{
	char *name, *devtype, *path, *s;
	struct cg *cg;
	pid_t pid=0;
	int err=0, i;
	struct stat statbuf;
	char buf[512];
	struct jlhead *devs;
	
	if(jelopt(argv, 'h', "help", NULL, &err)) {
		printf("nscgdev [-p --pid NSPID] [-h] DEVPATH ..\n"
			);
		exit(0);
	}

	if(jelopt_int(argv, 'p', "pid",
		      &pid, &err))
		;

	argc = jelopt_final(argv, &err);

	if(pid == 0) {
		name = getenv("NSPID");
		if(!name) exit(2);
	} else {
		sprintf(buf, "%u", pid);
		name = strdup(buf);
	}
	
	cg = cgroup_access(name, &err);
	if(err) {
		printf("failed to access cgroup %s: %d\n", name, err);
		exit(2);
	}

	
	devs = jl_new();
	
	for(i=1;i<argc;i++) {
		path = argv[i];
		
		/* read major minor from devicenode */
		if(stat(path, &statbuf)) {
			fprintf(stderr, "Could not stat %s\n", path);
			exit(1);
		}
		
		devtype = NULL;
		if(S_ISBLK(statbuf.st_mode))
			devtype = "b";
		if(S_ISCHR(statbuf.st_mode))
			devtype = "c";
		
		if(!devtype) {
			fprintf(stderr, "Not a devicenode %s\n", path);
			exit(1);    
		}
		
		sprintf(buf, "%s %d:%d rwm", 
			devtype, 
			major(statbuf.st_rdev), 
			minor(statbuf.st_rdev));
		
		jl_append(devs, strdup(buf));
	}
	
	jl_foreach(devs, s) {
		/* echo 'c 1:3 mr' > /cgroups/1/devices.allow */
		cgroup_put(cg, "devices.allow", s);
	}
	
	printf("ok\n");
	exit(0);
}
