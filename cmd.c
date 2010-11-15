/*
 * File: cmd.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "cmd.h"

static char **argv_build(const char *cmd)
{
  int n=2;
  const char *p;
  int i;
  char **va;
  const char *start;
  
  for(p=cmd;*p;p++)
    if(*p == ' ') n++;
  
  va = malloc(sizeof(char*)*n);

  if(!va) return NULL;

  for(i=0,start=cmd,p=cmd;;) {
    if(*p == ' '||*p == 0) {
      va[i++] = strndup(start, p-start);
      if(*p == 0) break;
      while(*p == ' ') p++;
      start = p;
      continue;
    }
    if(*p == 0) break;
    p++;
  }
  va[i] = NULL;
  return va;
}

static char **env_build(const char **envp, const char *envadd)
{
  int n, len=0, i;
  char **va, *p;
  
  for(n=0;envp[n];n++);
  n+=2;
  
  va = malloc(sizeof(char*)*n);

  if(!va) return NULL;

  if(envadd) {
    p = strchr(envadd, '=');
    if(p) {
      len = p-envadd;
    } else {
      envadd = NULL;
    }
  }
  
  if(!envadd) {
    for(n=0;envp[n];n++)
      va[n] = strdup(envp[n]);
    va[n] = NULL;
    return va;
  }

  for(i=0,n=0;envp[i];i++) {
    if(strncmp(envp[i], envadd, len)) {
      va[n++] = strdup(envp[i]);
    }
  }
  va[n++] = strdup(envadd);
  va[n] = NULL;

  return va;
}

int exec_cmd(const char *cmd, const char **envp, const char *envadd, uid_t uid, gid_t gid)
{
  int pfd[2], n, status;
  pid_t pid;
  char **argv, **env;
  char buf[2];

  pipe(pfd);
  fcntl(pfd[1], F_SETFD, (long)FD_CLOEXEC);

  pid = fork();
  if(pid==-1) return -1;
  if(pid==0)
    {
      argv = argv_build(cmd);
      env = env_build(envp, envadd);
      close(0);
      open("/dev/null", O_RDONLY);
      close(pfd[0]);

      if(setresuid(uid, uid, uid)) goto errout;
      if(setresgid(gid, gid, gid)) goto errout;

      execve(argv[0], argv, env);
    errout:
      write(pfd[1], "E", 1);
      exit(99);
    }
  close(pfd[1]);
  n = read(pfd[0], buf, 1);
  close(pfd[0]);
  if(n==1) {
    /* we know exec failed */
    waitpid(pid, &status, 0);
    return -1;
  }
  
  waitpid(pid, &status, 0);
  if(!WIFEXITED(status))
    return -2;
  
  return WEXITSTATUS(status);
}
