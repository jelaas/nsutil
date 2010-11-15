/*
 * lxc: linux Container library
 *
 * (C) Copyright IBM Corp. 2007, 2008
 *
 * Authors:
 * Daniel Lezcano <dlezcano at fr.ibm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include "lxc_macvlan_create.h"

#ifndef IFLA_LINKMODE
#  define IFLA_LINKMODE 17
#endif

#ifndef IFLA_LINKINFO
#  define IFLA_LINKINFO 18
#endif

#ifndef IFLA_NET_NS_PID
#  define IFLA_NET_NS_PID 19
#endif

#ifndef IFLA_INFO_KIND
# define IFLA_INFO_KIND 1
#endif

#ifndef IFLA_INFO_DATA
#  define IFLA_INFO_DATA 2
#endif

#ifndef IFLA_MACVLAN_MODE
# define IFLA_MACVLAN_MODE 1
#endif

struct link_req {
	struct nlmsg nlmsg;
	struct ifinfomsg ifinfomsg;
};

int lxc_macvlan_create(const char *master, const char *name, int mode)
{
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL, *answer = NULL;
	struct link_req *link_req;
	struct rtattr *nest, *nest2;
	int index, len, err;

	err = netlink_open(&nlh, NETLINK_ROUTE);
	if (err)
		return err;

	err = -EINVAL;
	len = strlen(master);
	if (len == 1 || len > IFNAMSIZ)
		goto out;

	len = strlen(name);
	if (len == 1 || len > IFNAMSIZ)
		goto out;

	err = -ENOMEM;
	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg)
		goto out;

	answer = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!answer)
		goto out;

	err = -EINVAL;
	index = if_nametoindex(master);
	if (!index)
		goto out;

	link_req = (struct link_req *)nlmsg;
	link_req->ifinfomsg.ifi_family = AF_UNSPEC;
	nlmsg->nlmsghdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlmsg->nlmsghdr.nlmsg_flags =
		NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK;
	nlmsg->nlmsghdr.nlmsg_type = RTM_NEWLINK;

	nest = nla_begin_nested(nlmsg, IFLA_LINKINFO);
	if (!nest)
		goto out;

	if (nla_put_string(nlmsg, IFLA_INFO_KIND, "macvlan"))
		goto out;

	if (mode) {
		nest2 = nla_begin_nested(nlmsg, IFLA_INFO_DATA);
		if (!nest2)
			goto out;

		if (nla_put_u32(nlmsg, IFLA_MACVLAN_MODE, mode))
			goto out;

		nla_end_nested(nlmsg, nest2);
	}

	nla_end_nested(nlmsg, nest);

	if (nla_put_u32(nlmsg, IFLA_LINK, index))
		goto out;

	if (nla_put_string(nlmsg, IFLA_IFNAME, name))
		goto out;

	err = netlink_transaction(&nlh, nlmsg, answer);
out:
	netlink_close(&nlh);
	nlmsg_free(answer);
	nlmsg_free(nlmsg);
	return err;
}
