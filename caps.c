/*
 * File: caps.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

/*
  f: file
       CAP_CHOWN
       CAP_FOWNER
       CAP_FSETID
       CAP_LINUX_IMMUTABLE
       CAP_DAC_OVERRIDE
       CAP_DAC_READ_SEARCH
       CAP_LEASE (since Linux 2.4)

 k: kill
       CAP_KILL

 m: mac security
       CAP_MAC_ADMIN (since Linux 2.6.25)
       CAP_MAC_OVERRIDE (since Linux 2.6.25)

 i: ipc
       CAP_IPC_LOCK
       CAP_IPC_OWNER

 n: net
       CAP_NET_ADMIN
       CAP_NET_BIND_SERVICE
       CAP_NET_BROADCAST
       CAP_NET_RAW

 c: capabilities
       CAP_SETFCAP (since Linux 2.6.24)
       CAP_SETPCAP

 u: uid/gid manipul
       CAP_SETUID
       CAP_SETGID

 b: boot
       CAP_SYS_BOOT

 s: sys
       CAP_SYS_ADMIN
       CAP_SYS_CHROOT
       CAP_SYS_MODULE
       CAP_SYS_NICE
       CAP_SYS_PACCT
       CAP_SYS_PTRACE
       CAP_SYS_RAWIO
       CAP_SYS_RESOURCE
       CAP_SYS_TTY_CONFIG
       CAP_MKNOD (since Linux 2.4)
       CAP_AUDIT_CONTROL (since Linux 2.6.11)
       CAP_AUDIT_WRITE (since Linux 2.6.11)

 t: time
       CAP_SYS_TIME
 */

#include <sys/prctl.h>
#include <sys/capability.h>

#include "caps.h"

uint64_t cap_set(const char *tokens)
{
  uint64_t caps = 0;
  while(*tokens) {
    if(*tokens == 'b')
      caps += (1<<CAP_SYS_BOOT);

    if(*tokens == 'c')
      caps += (1<<CAP_SETFCAP)|(1<<CAP_SETPCAP);

    if(*tokens == 'f')
      caps += (1<<CAP_CHOWN)|(1<<CAP_FOWNER)|(1<<CAP_FSETID)|
	(1<<CAP_LINUX_IMMUTABLE)|
	(1<<CAP_DAC_OVERRIDE)|(1<<CAP_DAC_READ_SEARCH)|(1<<CAP_LEASE);

    if(*tokens == 'i')
      caps += (1<<CAP_IPC_LOCK)|(1<<CAP_IPC_OWNER);

    if(*tokens == 'k')
      caps += (1<<CAP_KILL);

    if(*tokens == 'm')
      caps += ((uint64_t)1<<CAP_MAC_ADMIN)|((uint64_t)1<<CAP_MAC_OVERRIDE);

    if(*tokens == 'n')
      caps += (1<<CAP_NET_ADMIN)|(1<<CAP_NET_BIND_SERVICE)|
	(1<<CAP_NET_BROADCAST)|(1<<CAP_NET_RAW);

    if(*tokens == 's')
      caps += (1<<CAP_SYS_ADMIN)|(1<<CAP_SYS_CHROOT)|(1<<CAP_SYS_MODULE)|
	(1<<CAP_SYS_NICE)|(1<<CAP_SYS_PACCT)|(1<<CAP_SYS_PTRACE)|
	(1<<CAP_SYS_RAWIO)|
	(1<<CAP_SYS_RESOURCE)|(1<<CAP_SYS_TTY_CONFIG)|(1<<CAP_MKNOD)|
	(1<<CAP_AUDIT_CONTROL)|(1<<CAP_AUDIT_WRITE);

    if(*tokens == 't')
      caps += (1<<CAP_SYS_TIME);

    if(*tokens == 'u')
      caps += (1<<CAP_SETUID)|(1<<CAP_SETGID);

    tokens++;
  }
  return caps;
}

int cap_drop(uint64_t caps)
{
  int i;
  int rc=0;
  
  for(i=0;i<=CAP_LAST_CAP;i++) {
    if(caps & ((uint64_t)1<<i)) {
      rc = prctl(PR_CAPBSET_DROP, i, 0, 0, 0);
      if(rc) return i+1;
    }
  }
  return 0;
}
