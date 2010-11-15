#ifndef NSUTIL_CMD_H
#define NSUTIL_CMD_H

#include <sys/types.h>
#include <unistd.h>
int exec_cmd(const char *cmd, const char **envp, const char *envadd, uid_t uid, gid_t gid);

#endif
