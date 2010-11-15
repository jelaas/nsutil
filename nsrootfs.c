/*
 * File: nsrootfs.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

/*

  nsrootfs [--chroot|-pivot_root]  /NEWROOT CMD [ARG ..] 


 */
#define _GNU_SOURCE

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int do_chroot(const char *newroot)
{
  if(chroot(newroot))
    return -1;
  chdir("/");
  return 0;
}

#define PUTOLD "/put_old"
int do_pivot(const char *newroot)
{
  char *put_old;

  put_old = malloc(strlen(newroot)+strlen(PUTOLD)+2);
  sprintf(put_old, "%s%s", newroot, PUTOLD);

  chdir(newroot);
  if(pivot_root(newroot, put_old)) {
    fprintf(stderr, "errno=%d\n", errno);
    fprintf(stderr, "failed pivot_root(%s, %s): %s\n", newroot, put_old, strerror(errno));
    return errno;
  }
    
  chdir("/");
  return 0;
}

int main(int argc, char **argv)
{
  uid_t ruid, euid, suid;
  gid_t rgid, egid, sgid;
  int rc;

  /* change uid,gid back to user when running as suid root */
  getresuid(&ruid, &euid, &suid);
  getresgid(&rgid, &egid, &sgid);

  if(0 && (rc=do_pivot(argv[1]))) {
    exit(2);
  }
  
  if(do_chroot(argv[1]))
    exit(2);

  if(setresuid(ruid, ruid, ruid)) {
    exit(2);
  }
  if(setresgid(rgid, rgid, rgid)) exit(2);

  if(argc<3) {
    execl("/bin/bash", "/bin/bash", "-sh", NULL);
    exit(2);
  }

  /* FIXME: add Host to Guest pipes support or intruder support.
     some way for Host to run programs inside chroot namespace
   */
  
  argv+=2;
  execvp(argv[0], argv);
  fprintf(stderr, "exec %s failed\n", argv[0]);
  exit(2);
}
