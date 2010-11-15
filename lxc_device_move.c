/*
 * File: lxc_device_move.c
 * Implements:
 *
 * Copyright: Jens Låås, 2009
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

/*
 * lxc: linux Container library
 *
 * (C) Copyright IBM Corp. 2007, 2008
 *
 * Authors:
 * Daniel Lezcano <dlezcano at fr.ibm.com>
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#undef _GNU_SOURCe
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <linux/if_bridge.h>
#include "nl.h"

struct link_req {
  struct nlmsg nlmsg;
  struct ifinfomsg ifinfomsg;
};


int lxc_device_move(int ifindex, pid_t pid)
{
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL;
	struct link_req *link_req;
	int err = -1;

	if (netlink_open(&nlh, NETLINK_ROUTE))
		return -1;

	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg) {
		err = -2;
		goto out;
	}

	link_req = (struct link_req *)nlmsg;
	link_req->ifinfomsg.ifi_family = AF_UNSPEC;
	link_req->ifinfomsg.ifi_index = ifindex;
	nlmsg->nlmsghdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlmsg->nlmsghdr.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;
	nlmsg->nlmsghdr.nlmsg_type = RTM_NEWLINK;

	if (nla_put_u32(nlmsg, IFLA_NET_NS_PID, pid)) {
		err = -3;
		goto out;
	}

	if (netlink_transaction(&nlh, nlmsg, nlmsg)) {
		err = -4;
		goto out;
	}

	err = 0;
out:
	netlink_close(&nlh);
	nlmsg_free(nlmsg);
	return err;
}
