/*
 * File: lxc_device_rename_byindex.c
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

int lxc_device_rename_byindex(int ifindex, const char *newname)
{
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL, *answer = NULL;
	struct link_req *link_req;
	int index, len, err = -1;

	if (netlink_open(&nlh, NETLINK_ROUTE))
		return -1;

	len = strlen(newname);
	if (len == 1 || len > IFNAMSIZ)
		goto out;

	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg)
		goto out;

	answer = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!answer)
		goto out;

	index = ifindex;
	if (!index)
		goto out;

	link_req = (struct link_req *)nlmsg;
	link_req->ifinfomsg.ifi_family = AF_UNSPEC;
	link_req->ifinfomsg.ifi_index = index;
	nlmsg->nlmsghdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlmsg->nlmsghdr.nlmsg_flags = NLM_F_ACK|NLM_F_REQUEST;
	nlmsg->nlmsghdr.nlmsg_type = RTM_NEWLINK;

	if (nla_put_string(nlmsg, IFLA_IFNAME, newname))
		goto out;

	if (netlink_transaction(&nlh, nlmsg, answer))
		goto out;

	err = 0;
out:
	netlink_close(&nlh);
	nlmsg_free(answer);
	nlmsg_free(nlmsg);
	return err;
}
