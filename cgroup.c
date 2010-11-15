/*
 * File: cgroup.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include "cgroup.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/mount.h>
#include <syscall.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sched.h>
#include <ctype.h>
#include <sys/vfs.h>

#include <linux/types.h>

static int try(const char *path, char *buf)
{
  buf[0] = 0;
  if(access(path, R_OK|X_OK))
    return -1;
  strcpy(buf, path);
  return 0;
}

static int cgroup_open(const struct cg *cg, const char *name)
{
  char buf[512];

  sprintf(buf, "%s/%s", cg->path, name);
  return open(buf, O_RDWR);
}

ssize_t cgroup_get(const struct cg *cg, char *buf, size_t buflen, const char *name)
{
  int fd;
  ssize_t n;

  buf[0] = 0;
  fd = cgroup_open(cg, name);

  if(fd == -1)
    return -1;

  n = read(fd, buf, buflen);
  close(fd);
  return n;
}

int cgroup_put(const struct cg *cg, const char *name, const char *value)
{
  int fd;

  fd = cgroup_open(cg, name);

  if(fd == -1)
    return -1;

  if(write(fd, value, strlen(value))!=strlen(value)) {
    close(fd);
    return -2;
  }

  close(fd);
  return 0;
}

struct cg *cgroup_access(const char *name, int *err)
{
  char dir[512], buf[512];
  struct cg *cg;
  struct statfs statbuf;
  
  if(err) *err = 0;
  
  /* find a mountpoint */
  try("/dev/cgroup", dir);

  if(statfs(dir, &statbuf))
    {
      if(err) *err = 1;
      return NULL;
    }

  if(statbuf.f_type != 0x27e0eb) {
    //    printf("type: %x\n", statbuf.f_type);
      if(err) *err = 2;
    return NULL;
  }

  if(!dir[0]) {
    if(err) *err = 3;
    return NULL;
  }

  /* create cgroup if needed */
  cg = malloc(sizeof(struct cg));
  if(!cg) {
    if(err) *err = 4;
    return NULL;
  }
  cg->path[0] = 0;

  sprintf(cg->path, "%s/%s", dir, name);
  if(access(cg->path, R_OK|X_OK)) {
    /* need to create */
    if(mkdir(cg->path, 0777)) {
      if(err) *err = 5;
      return NULL;
    }
  }

  /* set cpuset.cpus and cpuset.mems if needed */
  cgroup_get(cg, buf, sizeof(buf), "cpuset.cpus");
  if(!isdigit(buf[0]))
     cgroup_put(cg, "cpuset.cpus", "0");

  cgroup_get(cg, buf, sizeof(buf), "cpuset.mems");
  if(!isdigit(buf[0]))
     cgroup_put(cg, "cpuset.mems", "0");

  return cg;
}
