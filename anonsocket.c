/*
 * File: anonsocket.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include "anonsocket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>

int anonsocket(const char *name)
{
	int fd;
	struct sockaddr_un sun;

	/* Setup for abstract unix socket */

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	sun.sun_path[0] = 0;
	sprintf(sun.sun_path+1, "%s", name);

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	if (bind(fd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
		return -2;
	}
	if (listen(fd, 5) < 0) {
		return -3;
	}
	return fd;
}
