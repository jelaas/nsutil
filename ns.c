/*
 * File: ns.c
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
#include <sys/prctl.h>
#include <sys/capability.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <syslog.h>

#include <stdint.h>

#include "cmd.h"
#include "caps.h"
#include "anonsocket.h"

#include "jelist.h"
#include "jelopt.h"

#define NSPIDLEN 17

#ifndef CLONE_FS
#  define CLONE_FS                0x00000200
#endif
#ifndef CLONE_NEWNS
#  define CLONE_NEWNS             0x00020000
#endif
#ifndef CLONE_NEWUTS
#  define CLONE_NEWUTS            0x04000000
#endif
#ifndef CLONE_NEWIPC
#  define CLONE_NEWIPC            0x08000000
#endif
#ifndef CLONE_NEWUSER
#  define CLONE_NEWUSER           0x10000000
#endif
#ifndef CLONE_NEWPID
#  define CLONE_NEWPID            0x20000000
#endif
#ifndef CLONE_NEWNET        
#  define CLONE_NEWNET            0x40000000
#endif                      


struct args {
	int argc;
	char **argv;
	const char **envp;
	int parentfd, closefd;
	int execfd, closefd2;
	struct jlhead *cmd;
};

struct {
	int daemon;
	int debug;
	int fd;
	char *capset;
	char *name;
	char *log;
	FILE *logf;
} conf;

static void msg(const char *fmt, ...)
{
	va_list ap;
  
	va_start(ap, fmt);
	
	if(conf.logf) {
		fprintf(conf.logf, "ns[%s][%u]: ", conf.name?conf.name:"", getpid());
		vfprintf(conf.logf, fmt, ap);
		va_end(ap);
		return;
	}
	if(conf.daemon) {
		vsyslog(LOG_DAEMON|LOG_ERR, fmt, ap);
		va_end(ap);
		return;
	}
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	return;
}

int child(void *arg)
{
	struct args *args = arg;
	uid_t ruid, euid, suid;
	gid_t rgid, egid, sgid;
	char buf[2], nspid_str[NSPIDLEN+1], envadd[32], *cmd;
	int rc;
	
	if(conf.debug > 1) msg("namespace child started\n");
	
	if(!conf.daemon)
		if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0)) {
			if(conf.debug) msg("prctl(PR_SET_PDEATHSIG, SIGKILL) failed.\n");
			exit(2);
		}
	
	close(args->closefd);
	close(args->closefd2);
	fcntl(args->execfd, F_SETFD, (long)FD_CLOEXEC);

	/*
	  wait for notification via pipe until we continue, must give
	  parent time to setup
	*/
	if(read(args->parentfd, buf, 1)!=1)
		exit(2);
	
	if(buf[0] != 'O') exit(2);
	
	if(read(args->parentfd, nspid_str, NSPIDLEN)!=NSPIDLEN)
		exit(2);

	close(args->parentfd);
	
	/* change uid,gid back to user when running as suid root */
	getresuid(&ruid, &euid, &suid);
	getresgid(&rgid, &egid, &sgid);
	
	/*
	  we should run cmd after changing uid etc
	*/
	sprintf(envadd, "NSPID=0");
	jl_foreach(args->cmd, cmd) {
		/* FIXME: clear close on exec on pipes (Guest end)
		   When cmd is prefixed by "<pipename>:"
		   Add H2G_<pipe> env to envp
		 */
		if(exec_cmd(cmd, args->envp, envadd, ruid, rgid)) {
			if(conf.debug) msg("exec_cmd failed from inside ns.\n");
			exit(2);
		}
		/* FIXME: reinstate close on exec on pipes */
	}
	
	/* drop capabilities the last thing we do */
	if(conf.capset) {
		if ((rc=cap_drop(cap_set(conf.capset)))) {
			if(conf.debug) msg("Failed to drop capability %d.\n", rc-1);
			exit(2);
		}
	}
	
	if(setresuid(ruid, ruid, ruid)) {
		if(conf.debug) msg("setresuid failed.\n");
		exit(2);
	}
	if(setresgid(rgid, rgid, rgid)) exit(2);
	
	if (args->argc<2) {
		setenv("NSPID", nspid_str, 1);
		execl("/bin/bash", "/bin/bash", "-sh", NULL);
		write(args->execfd, "E", 1);
		exit(2);
	}

	args->argv++;

	/* FIXME: clear close on exec on pipes (Guest end)
	   When argv[0] is prefixed by "<pipename>:" ??
	   Add H2G_<pipe> env
	*/
	
	execvp(args->argv[0], args->argv);
	write(args->execfd, "E", 1);
	exit(2);
}

int main(int argc, char **argv, const char **envp)
{
	pid_t pid;
	int flags = 0;
	int status;
	struct args args;
	long stack_size = sysconf(_SC_PAGESIZE);
	void *stack = alloca(stack_size) + stack_size;
	struct jlhead *nsruncmd, *hruncmd, *pipes;
	int rc=0, err=0;
	int pfd[2], efd[2];
	char envadd[32], *cmd;
	char nspid_str[NSPIDLEN+1];
	
	conf.fd = -1;
	
	nsruncmd = jl_new();
	hruncmd = jl_new();
	pipes = jl_new();
	
	if(jelopt(argv, 'h', "help", NULL, &err)) {
		printf("ns [-dv] [-t --type NS] [-n --name NAME][-c|--dropcaps CAPSET][-N|--nsrun CMD] [-H|--hrun CMD] [--log LOG] [-- [pipe:]CMD [ARGS ..]]\n"
		       "\n"
		       " -d --daemonize\n"
		       " -t --type net|vfs|pid|uts|ipc|user|all\n"
		       " --dropcaps [fkmincubst]\n"
		       " -N --nsrun [pipe:]CMD\n"
		       " -H --hrun CMD\n"
		       " -p --pipe NAME - Create a pipe between host and guest. Accessed via fds ENV['H2G_<name>'] = fd,fd\n"
		       " --log LOGFILE\n"
		       "\n"
		       "After the child is created, parent will execute all --hrun commands.\n"
		       "Meanwhile the child waits until parent signals that it is done. This\n"
		       " is done via a pipe.\n"
		       "After receiving the signal the child runs all --nsrun commands.\n"
		       "\n"
		       "Capability tokens (used in --dropcaps):\n"
		       "f: file CAP_CHOWN CAP_FOWNER CAP_FSETID CAP_LINUX_IMMUTABLE CAP_DAC_OVERRIDE\n"
		       "        CAP_DAC_READ_SEARCH CAP_LEASE\n"
		       "k: kill CAP_KILL\n"
		       "m: mac security CAP_MAC_ADMIN CAP_MAC_OVERRIDE\n"
		       "i: ipc CAP_IPC_LOCK CAP_IPC_OWNER\n"
		       "n: net CAP_NET_ADMIN CAP_NET_BIND_SERVICE CAP_NET_BROADCAST CAP_NET_RAW\n"
		       "c: capabilities CAP_SETFCAP CAP_SETPCAP\n"
		       "u: uid/gid CAP_SETUID CAP_SETGID\n"
		       "b: boot CAP_SYS_BOOT\n"
		       "s: sys CAP_SYS_ADMIN CAP_SYS_CHROOT CAP_SYS_MODULE CAP_SYS_NICE CAP_SYS_PACCT\n"
		       "       CAP_SYS_PTRACE CAP_SYS_RAWIO CAP_SYS_RESOURCE CAP_SYS_TTY_CONFIG\n"
		       "       CAP_MKNOD CAP_AUDIT_CONTROL CAP_AUDIT_WRITE\n"
		       "t: time CAP_SYS_TIME\n"
		       "\n"
		       "Examples:\n"
		       "Create a new net namespace and assign eth0:\n"
		       " ns -t net --hrun \"/sbin/nsnetif -i eth0\"\n"
			);
		exit(0);
	}
	
	while(jelopt(argv, 'd', "daemonize",
		     NULL, &err))
		conf.daemon = 1;
	
	while(jelopt(argv, 'v', "debug",
		     NULL, &err))
		conf.debug++;
	
	while(jelopt(argv, 'c', "dropcaps",
		     &conf.capset, &err));
	
	while(jelopt(argv, 0, "log",
		     &conf.log, &err));
	
	while(jelopt(argv, 'p', "pipe",
		     &cmd, &err)) {
		jl_append(pipes, strdup(cmd));
	}
	
	while(jelopt(argv, 'n', "name",
		     &conf.name, &err));
	
	while(jelopt(argv, 't', "type",
		     &cmd, &err)) {
		if(strstr(cmd, "net")) flags |= CLONE_NEWNET;
		if(strstr(cmd, "vfs")) flags |= CLONE_NEWNS;
		if(strstr(cmd, "pid")) flags |= CLONE_NEWPID;
		if(strstr(cmd, "uts")) flags |= CLONE_NEWUTS;
		if(strstr(cmd, "ipc")) flags |= CLONE_NEWIPC;
		if(strstr(cmd, "user")) flags |= CLONE_NEWUSER;
		if(strstr(cmd, "all")) flags |= CLONE_NEWNET|CLONE_NEWNS|CLONE_NEWPID|CLONE_NEWUTS|CLONE_NEWIPC|CLONE_NEWUSER;
	}
	
	while(jelopt(argv, 'N', "nsrun",
		     &cmd, &err)) {
		jl_append(nsruncmd, strdup(cmd));
	}
	while(jelopt(argv, 'H', "hrun",
		     &cmd, &err)) {
		jl_append(hruncmd, strdup(cmd));
	}
	
	argc = jelopt_final(argv, &err);
	
	if(conf.log) conf.logf = fopen(conf.log, "a");
	else {
		if(conf.daemon) {
			char ident[256];
			sprintf(ident, "ns[%s]", conf.name?conf.name:"");
			openlog(ident, LOG_PID, LOG_DAEMON);
		}
	}
	
	if(err) {
		msg("Bailing out.\n");
		exit(2);
	}
	
	if(conf.daemon) {
		int fd;
		
		if(fork()) _exit(0);
		umask(0);
		setsid();
		if((fd = open("/dev/null", O_RDWR, 0)) >= 0) {
			if(fd>2) {
				dup2(fd, 0);
				dup2(fd, 1);
				dup2(fd, 2);
			}
			if(fd) close(fd);
		}
	}
	
	args.argc = argc;
	args.argv = argv;
	args.envp = envp;
	args.cmd = nsruncmd;
	
	/*  Make pipes here to communicate with child */
	pipe(pfd);
	pipe(efd);
	args.parentfd = pfd[0];
	args.closefd = pfd[1];
	args.execfd = efd[1];
	args.closefd2 = efd[0];
	
	/* Create Host to Guest pipes. Set CLOSE_ON_EXEC flag on all */
	
	pid = clone(child, stack, flags | SIGCHLD, &args);
	close(pfd[0]);
	close(efd[1]);
	if(pid == -1) {
		msg("clone() failed\n");
		exit(1);
	}

	/* close Guest end of all pipes */
	
	if(pid > 0) {
		char buf[2];
		int n;
		
		/*
		  run commands to be executed before child continues in the new namespace
		*/
		sprintf(envadd, "NSPID=%u", pid);
		jl_foreach(hruncmd, cmd) {
			if(exec_cmd(cmd, envp, envadd, getuid(), getgid())) {
				msg("exec_cmd(%s) failed\n", cmd);
				rc = 2;
				break;
			}
		}
		
		/*
		  Tell child to continue via pipe.
		*/
		if(rc)
			write(pfd[1], "E", 1);
		else
		{
			// FIXME: we should detect if the child died. or catch sigpipe
			write(pfd[1], "O", 1);
			sprintf(nspid_str, "%u", pid);
			write(pfd[1], nspid_str, NSPIDLEN);
		}
		close(pfd[1]);
		
		n = read(efd[0], buf, 1);
		close(efd[0]);
		if(n==1) {
			/* we know exec failed */
			msg("child exec(%s): failed\n", argv[1]);
			waitpid(pid, &status, 0);
			exit(2);
		}
		
		close(0);
		close(1);
		if(conf.daemon||(conf.debug==0)) close(2);
		
		{
			char sockname[256];
			int id=1;
			while(1) {
				sprintf(sockname, "namespace_%d", id);
				if((conf.fd=anonsocket(sockname))>=0) {
					break;
				}
			}
		}
		
		chdir("/");
		if(conf.debug) msg("waiting for child %u\n", pid);
		waitpid(pid, &status, 0);
		if(WIFEXITED(status)) exit(WEXITSTATUS(status));
		exit(3);
	}
	
	exit(0);
}

/*

 ns -t net --init "nsnetif --rename apa eth0" --post "nsnetif -i apa" bash

 which means
    run "nsnetif --rename apa eth0" inside the new ns
    run "nsnetif -i apa" in the parent to export if to ns. pid is exported
      to --post command via ENV[NSPID].
    finally exec "bash" in the new ns.
    Sync is used to ensure that --init is run after --post has completed.
 */
