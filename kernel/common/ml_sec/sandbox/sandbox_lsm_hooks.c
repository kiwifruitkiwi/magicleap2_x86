/*
 * Copyright (c) 2017, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * This file contains the implementations of the syscall inspection
 * functions, as well as their function table.
 */

#include <linux/ml_sec/sandbox.h>
#include <linux/init.h>
#include <linux/lsm_hooks.h>
#include <linux/inetdevice.h>
#include <net/af_unix.h>
#include <net/ipv6.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/compiler.h>
#include <linux/binfmts.h>
#include "sandbox_profile_tags.h"


/*#define DEBUG_SOCK */

#ifdef DEBUG_SOCK
/* this is used for debugging localhost connection functionality
 * enable logging by doing:
 * echo Y > /sys/module/sandbox_lsm_hooks/parameters/log_net_debug
 */
bool log_net_debug; /* =false */
module_param(log_net_debug, bool, 0644);

static void debug_sock_log(struct sock *sk, struct sk_buff *skb, bool with_tcp, const char *msg, ...)
{
	char comm[TASK_COMM_LEN] = {0};
	char msg_buf[256];
	va_list args;

	if (!log_net_debug)
		return;
	if (current == NULL) /* sanity */
		return;
	if (sk != NULL && (sk->sk_family != PF_INET && sk->sk_family != PF_INET6))
		return;

	va_start(args, msg);
	vsnprintf(msg_buf, 256, msg, args);
	va_end(args);

	if (with_tcp && sk != NULL && skb != NULL) {
		unsigned char *sa = (unsigned char *)&ip_hdr(skb)->saddr, *da = (unsigned char *)&ip_hdr(skb)->daddr;

		pr_err("~~~~~ %s  tgid=%d pid=%d (%s)  skb=%pK  sk=%pK sk_fam=%d  protocol=%d src=%d.%d.%d.%d:%d dest=%d.%d.%d.%d:%d  syn=%d ack=%d rst=%d fin=%d len=%d : sending_tgid=(skb:%x/sk:%x)\n",
			   msg_buf, task_tgid_nr(current), task_pid_nr(current), get_task_comm(comm, current),
			   skb, sk, sk->sk_family,
			   ip_hdr(skb)->protocol, sa[0], sa[1], sa[2], sa[3], ntohs(tcp_hdr(skb)->source), da[0], da[1], da[2], da[3], ntohs(tcp_hdr(skb)->dest),
			   tcp_hdr(skb)->syn, tcp_hdr(skb)->ack, tcp_hdr(skb)->rst, tcp_hdr(skb)->fin, ntohs(ip_hdr(skb)->tot_len),
			   skb->sending_tgid, sk->__sk_common.skc_creator_tgid);
	} else {
		pr_err("~~~~~ %s  tgid=%d pid=%d (%s)  skb=%pK  sk=%pK\n", msg_buf, task_tgid_nr(current), task_pid_nr(current), get_task_comm(comm, current), skb, sk);
	}
}

#define DEBUG_NET_LOG(fmt, ...) do { if (log_net_debug) pr_err("~~~~~ " fmt "\n", ##__VA_ARGS__); } while (0)
#define DEBUG_SOCK_LOG(msg, sk, skb, ...) debug_sock_log(sk, skb, false, msg, ##__VA_ARGS__)
#define DEBUG_SOCK_LOG_TCP(msg, sk, skb, ...) debug_sock_log(sk, skb, true, msg, ##__VA_ARGS__)
#else
#define DEBUG_NET_LOG(fmt, ...)
#define DEBUG_SOCK_LOG(msg, sk, skb, ...)
#define DEBUG_SOCK_LOG_TCP(msg, sk, skb, ...)
#endif

/* The upper 3 bits of sk->__sk_common.skc_creator_tgid contains these values*/

/* the value can have one the following 3 values which are not bit fields
 * bits 31,30 are treated as a numberic value that can have one of these values, or 0
 * which means it's a real network packet
 */

/* means the creator is an allow-same-proc process, The lower bits of skc_creator_tgid are
 * the actual tgid of the creator. The start time of the process is in skc_creator_start_time
 */
#define SKC_CREATOR_TGID_SAME_PROC (0x40000000)

/* creator process is allowed to initiate connection with any process
 */
#define SKC_CREATOR_TGID_ALLOW_ALL (0x10000000)

/* creator process is in a conn_group
 * the group bits the process is in are in field skc_creator_groups
 */
#define SKC_CREATOR_TGID_IN_GROUP (0x20000000)

/*
 * tgid field in struct sockets includes three fields:
 * groups - bit combination of all the group that the socket belong to
 * tgid - the pid (task->tgid) of the process that initialized the connections
 * creator_type - bit combination of local host permissions granted to client
 *
 * tgid field in struct sock gets the following bits form
 * ****##################@@@@@@@@ (total of 32 bits)
 * where:
 * * - creator type
 * # - tgid values
 * @ - group bits
 */


/* the number of bits kept for creator type
 */
#define NUM_CREATOR_TYPE_BITS 4

/* the bit mask of creator type.
 * alligned to left of skc_creator_tgid in struct sock
 */
#define CREATOR_TYPE_MASK \
	(((1 << NUM_CREATOR_TYPE_BITS) - 1) << (32 - NUM_CREATOR_TYPE_BITS))

#define GET_TGID_CREATOR_TYPE(tgid) ((tgid) & CREATOR_TYPE_MASK)

/* the maximal number of groups is the number of bits that
 * "allow_local_transmit" can hold
 */
#define MAX_NUM_GROUPS (SIZEOF_MEMBER(struct sandbox_profile,\
			    attributes.allow_local_transmit.bits) << 3)


/*
 * masks group bit field from tgid
 */
#define GROUPS_MASK ((1 << MAX_NUM_GROUPS) - 1)

#define TGID_SHIFT MAX_NUM_GROUPS

/*
 * mask the pid from tgid field (see comment above)
 */
#define TGID_MASK (~(CREATOR_TYPE_MASK | GROUPS_MASK))

/*
 * Return the tgid.
 */
#define GET_TGID(tgid) ((tgid) & TGID_MASK)

#define GET_GROUPS(tgid) ((tgid) & GROUPS_MASK)

#define BITS_PER_PID 16

/* assert that the part that hold pid in "skc_creator_tgid"
 * is large enough
 */
STATIC_ASSERT_PRF(
	((SIZEOF_MEMBER(struct sock, __sk_common.skc_creator_tgid) << 3) -
		MAX_NUM_GROUPS - NUM_CREATOR_TYPE_BITS) >= BITS_PER_PID,
		not_enough_bits_for_pid);

/* go to the socket network namespace, go over all the devices there */
/* check if the address is of one of the devices */
static bool find_ipv4_address(struct sock *sk, __be32 addr)
{
	struct net *net = sock_net(sk);
	struct net_device *dev;
	struct in_device *in_dev;
	bool retval = true;

	rcu_read_lock();
	for_each_netdev_rcu(net, dev) {
		if (dev->reg_state >= NETREG_UNREGISTERING)
			continue;

		in_dev = __in_dev_get_rcu(dev);
		if (!in_dev)
			continue;

		for_ifa(in_dev) {
			if (addr == ifa->ifa_address)
				goto out;
		} endfor_ifa(in_dev);
	}

	retval = false;
out:
	rcu_read_unlock();
	return retval;
}

static bool find_ipv6_address(struct sock *sk, const struct in6_addr *addr)
{
	struct net *net = sock_net(sk);
	struct net_device *dev;
	struct inet6_dev *in6_dev;
	struct inet6_ifaddr *ifa;
	bool retval = true;

	rcu_read_lock();
	for_each_netdev_rcu(net, dev) {
		if (dev->reg_state >= NETREG_UNREGISTERING)
			continue;

		in6_dev = rcu_dereference(dev->ip6_ptr);
		if (!in6_dev)
			continue;

		list_for_each_entry(ifa, &in6_dev->addr_list, if_list) {
			if (ipv6_addr_equal(addr, &ifa->addr))
				goto out;
		}
	}

	retval = false;
out:
	rcu_read_unlock();
	return retval;
}

static inline bool is_local_ipv4_addr(struct sock *sk, __be32 addr)
{
	return IN_LOOPBACK(ntohl(addr)) ||
	       addr == INADDR_ANY       ||
	       find_ipv4_address(sk, addr);
}

static bool is_local_ipv6_addr(struct sock *sk, const struct in6_addr *addr)
{
	if (ipv6_addr_v4mapped(addr))
		return is_local_ipv4_addr(sk, addr->s6_addr32[3]);

	return ipv6_addr_loopback(addr) ||
	       ipv6_addr_any(addr)      ||
	       find_ipv6_address(sk, addr);
}


static void mark_sock_with_current_process(struct sock *sk,
	const char *op_name, bool force)
{
	pid_t tgid;
	struct task_struct *p;
	u64 start_time = 0;

    /* we force a change to the sock credentials
     * when the sock is created
     */
	if ((force == false) &&
		GET_TGID_CREATOR_TYPE(sk->__sk_common.skc_creator_tgid) != 0)
		return; /* set it only on the first time for any socket */

	tgid = task_tgid_nr(current);

	rcu_read_lock();
	p = find_task_by_pid_ns(tgid, &init_pid_ns);
	if (p != NULL)
		start_time = p->start_time;
	rcu_read_unlock();

	tgid = ((tgid << TGID_SHIFT) & TGID_MASK);

	if (p != NULL) {
		sk->__sk_common.skc_creator_tgid |=
			(tgid | SKC_CREATOR_TGID_SAME_PROC);
		sk->__sk_common.skc_creator_start_time = start_time;
		DEBUG_SOCK_LOG("  marking conn_sameproc:%s %x--%llu", sk, NULL, op_name, sk->__sk_common.skc_creator_tgid, start_time);
	}
}

static void mark_sock_with_groups(struct sock *sk, int groups,
	const char *op_name, bool force)
{
	if ((force == false) &&
		GET_TGID_CREATOR_TYPE(sk->__sk_common.skc_creator_tgid) != 0)
		return; /* set it only on the first time for any socket */

	sk->__sk_common.skc_creator_tgid |=
		(SKC_CREATOR_TGID_IN_GROUP | (groups & GROUPS_MASK));
	DEBUG_SOCK_LOG("  marking conn_group:%s %x--%d", sk, NULL, op_name,
		sk->__sk_common.skc_creator_tgid, groups);
}

static void mark_sock_with_allow_all(struct sock *sk, const char *op_name,
	bool force)
{
	if ((force == false) &&
		GET_TGID_CREATOR_TYPE(sk->__sk_common.skc_creator_tgid) != 0)
		return; /* set it only on the first time for any socket */

	sk->__sk_common.skc_creator_tgid |= SKC_CREATOR_TGID_ALLOW_ALL;
	DEBUG_SOCK_LOG("  marking allow-all:%s  %x", sk, NULL, op_name, sk->__sk_common.skc_creator_tgid);
}

static int verify_sockaddr(struct sock *sk, struct sockaddr *address,
	int addrlen, const char *op_name, bool force_set_sock_cred)
{
	unsigned short port;
	const struct sandbox_profile *sp __maybe_unused;
	bool allow = false;

	if (sk->sk_family == PF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)address;

		if (addrlen < sizeof(struct sockaddr_in))
			return -EINVAL;

		if (!is_local_ipv4_addr(sk, addr4->sin_addr.s_addr))
			return 0;

		port = ntohs(addr4->sin_port);
	} else if (sk->sk_family == PF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)address;

		if (addrlen < SIN6_LEN_RFC2133)
			return -EINVAL;

		if (!is_local_ipv6_addr(sk, &addr6->sin6_addr))
			return 0;

		port = ntohs(addr6->sin6_port);
	} else
		return 0;
#ifdef CONFIG_MLSEC_ALLOW_SAME_PROCESS_LOCALHOST
    /*the first time we set socket credentials we need to force
     * creation in order to support bot "sameproc" and "groups"
     * credentials
     */
	if (GET_TGID_CREATOR_TYPE(sk->__sk_common.skc_creator_tgid) == 0)
		force_set_sock_cred = true;
#endif


#ifdef CONFIG_MLSEC_ALLOW_SAME_PROCESS_LOCALHOST
	/* only one of the following conditions will be true for any profile */
	/* this is checked in the profile parser */
	sp = current->sandbox_profile;
	if (sp->attributes.allow_local_conn_sameproc) {
		mark_sock_with_current_process(sk, op_name,
			force_set_sock_cred);
		allow = true; /* going to be checked afterwards if needed */
	}
	if (sp->attributes.allow_local_transmit.bits != 0) {
		mark_sock_with_groups(sk,
			sp->attributes.allow_local_transmit.bits, op_name,
			force_set_sock_cred);
		allow = true; /* going to be checked afterwards if needed */
	}
#endif

	if (!allow) {
		sbox_audit_jail_localhost(op_name, port);
		if (!sbox_is_permissive())
			return -EPERM;
	}

	return 0;
}

static int sbox_socket_connect(struct socket *sock, struct sockaddr *address, int addrlen)
{
	DEBUG_SOCK_LOG("sbox_socket_connect", sock->sk, NULL);
	if (current->sandbox_profile->attributes.allow_local_connections) {
		/* we need to know this is an allow-all socket so that marked incoming
		 * packets from would not be block
		 */
		if (sock->sk->sk_family == PF_INET || sock->sk->sk_family == PF_INET6)
			mark_sock_with_allow_all(sock->sk,
				"connect(allow)", true);

		return 0;
	}

	return verify_sockaddr(sock->sk, address, addrlen, "connect", true);
}

/* called for every packet sent, UDP and TCP */
static int sbox_socket_sendmsg(struct socket *sock, struct msghdr *msg, int size)
{
	struct sockaddr *address;
	int addrlen;

	DEBUG_SOCK_LOG("sbox_socket_sendmsg", sock->sk, NULL);
	if (current->sandbox_profile->attributes.allow_local_connections) {
		/* with UDP client, this is the first time we ever see this sock (no connect or bind) */
		mark_sock_with_allow_all(sock->sk, "sendmsg(allow)", false);
		return 0;
	}

	address = msg->msg_name;
	addrlen = msg->msg_namelen;
	if (!address || !addrlen)
		return 0;

	return verify_sockaddr(sock->sk, address, addrlen, "sendto", false);
}


/*** same-process and group localhost connections **
 * We want to allow a process with allow_local_conn_sameproc flag to connect
 * to itself using TCP over localhost but not to other processes
 * Each side of the connection has a struct sock. each process tags the sock
 * it created with its own tgid. This is done in bind(), connect() and sendmsg()
 * A skbuff (sock buffer) that leaves a sock is tagged with the tgid of that
 * sock. This happens in netfilter_local_out()
 * When the skbuff reaches the other side sock, the tgid in
 * the skbuff is checked against the tgid of the receiving sock. This is done
 * in socket_sock_rcv_skb() and inet_conn_request()
 * This solution is inspired by the un-merged "friend" sockets patch here:
 * https://lwn.net/Articles/511079/
 * - in addition to tgid, the start-time of the process is also to ensure a
 * positive identification of the process
 * - tgid is saved in the sock in __sk_common.skc_creator_tgid. it's saved in
 * struct sock_common and not in struct sock because netfilter_local_out()
 * can receive things that are not derived from struct sock in it's sk argument
 * (request_sock in syn-ack)
 * - conn_groups is an extension of this to allow connection between different
 * processes. All the processes in a group need to have the same
 * allow_local_conn_group_X_YYY flag. In this case, creator_tgid is set to
 * SKC_CREATOR_TGID_IN_GROUP which is not a real tgid and creator_start_time
 * is set to the groups bit-mask of the process which created the sock/skbuff
 */

/* called when a process calls bind(), mark creator socket */
static int __maybe_unused sbox_socket_bind(struct socket *sock, struct sockaddr *address, int addrlen)
{
	const struct sandbox_profile *sp;

	if (sock == NULL || sock->sk == NULL)
		return 0; /* sanity */
	if (sock->sk->sk_family != PF_INET && sock->sk->sk_family != PF_INET6)
		return 0; /* quick return for the common case of PF_UNIX */

	DEBUG_SOCK_LOG("sbox_socket_bind() port=%d", sock->sk, NULL, ntohs(((struct sockaddr_in *)address)->sin_port));

	/* mark the receiving socket with the tgid with the type of process that created it
	 * If it receives real network packets, the acks for those (for example) will get marked, This is ok since
	 * that mark will not be read by anybody
	 */
	sp = current->sandbox_profile;
	if (sp->attributes.allow_local_connections)
		mark_sock_with_allow_all(sock->sk, "bind(allow)", true);

	if (sp->attributes.allow_local_conn_sameproc)
		mark_sock_with_current_process(sock->sk, "bind(same)", true);

	if (sp->attributes.allow_local_receive.bits)
		mark_sock_with_groups(sock->sk,
			sp->attributes.allow_local_receive.bits,
			"bind(group)", true);

	return 0;
}



/*** tag outgoing packet according to the socket they originate from
 * calls when a process sends a packet
 * This can't be done in sbox_socket_sendmsg since we don't have skb there yet
 * This function can be called in the context of any process
 *
 * int ipv4 tcp, state->sk will be the request_sock. this happens in ip_build_and_send_pkt (taking sbk->sk)
 */
static unsigned int sbox_netfilter_local_out(void *priv,
					     struct sk_buff *skb,
					     const struct nf_hook_state *state)
{
	struct sock *sk;
	pid_t skc_creator_tgid;

	if (state == NULL) { /* sanity */
		return NF_ACCEPT;
	}

	/* skb->sk can be NULL (ipv6 tcp) so use state->sk */
	sk = state->sk;

	if (sk == NULL) { /* can be NULL (igmpv3_sendpack) */
		return NF_ACCEPT;
	}
	if (skb == NULL) { /* sanity */
		return NF_ACCEPT;
	}

	skc_creator_tgid = sk->__sk_common.skc_creator_tgid;


	DEBUG_SOCK_LOG_TCP("=== sbox_netfilter_local_out", sk, skb);

	/* record the sending process tgid according from the sock to the skbuff */
	/* marked socket? and unmarked skb? (syn-ack is marked in tcp_make_synack)*/
	if (skc_creator_tgid != 0 && skb->sending_tgid == 0) {
		/* we need to have both creator tgid and the creator start time to avoid the possibility
		 * of faking a creator. A process can create a socket, inherit it to a child process
		 * and exit. A new process with the same tgid can now be created after some time
		 * and that new process will be able to open a localhost socket to the child
		 * process
		 */
		DEBUG_SOCK_LOG("  marking-local-out %x--%llu", sk, skb, skc_creator_tgid, sk->__sk_common.skc_creator_start_time);
		skb->sending_tgid = skc_creator_tgid;
		skb->sending_start_time = sk->__sk_common.skc_creator_start_time;
	}

	return NF_ACCEPT;
}

static struct nf_hook_ops local_out_nfho_inet __maybe_unused = {
		.hook	   = sbox_netfilter_local_out,
		.hooknum	= NF_INET_LOCAL_OUT,
		.pf		 = PF_INET,
		.priority   = NF_IP_PRI_FIRST,
};
static struct nf_hook_ops local_out_nfho_inet6 __maybe_unused = {
		.hook	   = sbox_netfilter_local_out,
		.hooknum	= NF_INET_LOCAL_OUT,
		.pf		 = PF_INET6,
		.priority   = NF_IP_PRI_FIRST,
};

/*
 * copy the client socket credentials from an incoming packet to the request.
 * the credentials will be set to the server side socket later, in
 * sbox_inet_csk_clone
 */
static void update_req(struct request_sock *req, struct sk_buff *skb)
{
	if (req != NULL) {
		req->__req_common.skc_creator_tgid = skb->sending_tgid;
		req->__req_common.skc_creator_start_time =
			skb->sending_start_time;
	}
}



static bool check_localhost_skb_to_sock(struct sock *sk, struct sk_buff *skb, const char *op_name, struct request_sock *req)
{
	unsigned short port = 0; /* for audit logging */
	pid_t skb_type, sk_type;

	if (sk == NULL || skb == NULL)
		return 0; /* sanity */
	if (sk->sk_family != PF_INET && sk->sk_family != PF_INET6)
		return 0; /* quick return for the common case of PF_UNIX */

	DEBUG_SOCK_LOG_TCP("check_localhost_skb_to_sock: %s", sk, skb, op_name);

	if (skb->sending_tgid == 0) {
		/* packet coming from the real network or from a local socket that is always allowed */
		/* (packet going to the real network will never be checked here) */
		DEBUG_SOCK_LOG("  check-PASS(0) %s", sk, skb, op_name);
		return 0;
	}

	/* if the skb has non-zero sending_tgid it means it came from a local socket */

	sk_type = GET_TGID_CREATOR_TYPE(sk->__sk_common.skc_creator_tgid);
	skb_type = GET_TGID_CREATOR_TYPE(skb->sending_tgid);

	/* attempt to accept if both server and clients sides support groups */
	if ((skb_type & SKC_CREATOR_TGID_IN_GROUP) &&
		(sk_type & SKC_CREATOR_TGID_IN_GROUP)) {
		u64 common_croups = GET_GROUPS(skb->sending_tgid) &
			GET_GROUPS(sk->__sk_common.skc_creator_tgid);
		/* sent in a group, need to have a common group bit */
		if (common_croups != 0) {
			DEBUG_SOCK_LOG("  check-MATCH-group %s", sk, skb, op_name);
			/* update request with client credentials,
			 * the goal is that the paired socket (on the
			 * server side) will have the same credentials
			 * as the client side socket
			 */
			update_req(req, skb);
			return 0;
		}
	} else if (skb_type & SKC_CREATOR_TGID_SAME_PROC) {
		/* real tgid, process should be identical in both ends */
		if ((GET_TGID(sk->__sk_common.skc_creator_tgid) ==
				GET_TGID(skb->sending_tgid)) &&
			(skb->sending_start_time ==
				sk->__sk_common.skc_creator_start_time)) {
			DEBUG_SOCK_LOG("  check-MATCH-proc %s", sk, skb, op_name);
			/* update request with client credentials, will
			 * be used when the end point socket will be
			 * created
			 */
			update_req(req, skb);
			return 0;
		}
	}

	if ((skb_type & SKC_CREATOR_TGID_ALLOW_ALL) != 0) {
		/* packet came from a process that is allowed localhost connection
		 * such connection is allowed only for TCP and only if the initiator (who called connect())
		 * is the one that had ALLOW_ALL.
		 * In this case, we want to allow the same-proc process to break out and respond
		 * we get here from sbox_inet_conn_request() with the sock_request req
		 * creator_tgid in req is overridden to be ALLOW_ALL (overriding whatever it got from it's parent sock in sbox_inet_conn_request)
		 * Then this flag is copied to the forked sock in sbox_inet_csk_clone
		 * so that the forked sock can freely communicate with the other side, without affecting
		 * the parent sock.
		 */
		DEBUG_SOCK_LOG("  check-PASS-allowsk %s", sk, skb, op_name);
		update_req(req, skb);
		return 0;
	}


	DEBUG_SOCK_LOG("  check-BLOCK %s skb:%x--%llu sk:%x--%llu", sk, skb, op_name,
				   skb->sending_tgid, skb->sending_start_time,
				   sk->__sk_common.skc_creator_tgid, sk->__sk_common.skc_creator_start_time);


	/* this works for both ipv6 and ipv6 */
	port = ntohs(tcp_hdr(skb)->dest);

	sbox_audit_jail_localhost(op_name, port);
	if (!sbox_is_permissive())
		return -ECONNREFUSED;

	return 0;
}


/* check received buffer agrees with receiving socket */
/* can be called from a kernel worker process */
static int __maybe_unused sbox_socket_sock_rcv_skb(struct sock *sk, struct sk_buff *skb)
{
	return check_localhost_skb_to_sock(sk, skb, "rcv_skb", NULL);
}


/* this essentially means - here's the request_sock that was cloned from sk due to skb(SYN)*/
/* req is the start of a fork-socket from sk so it should get the same tgid of sk */
static int __maybe_unused sbox_inet_conn_request(struct sock *sk, struct sk_buff *skb, struct request_sock *req)
{
	DEBUG_NET_LOG("conn_request rsk=%pK   sk=%pK   skb=%pK", req, sk, skb);

	if (req->__req_common.skc_creator_tgid == 0) {
		req->__req_common.skc_creator_tgid =       sk->__sk_common.skc_creator_tgid;
		req->__req_common.skc_creator_start_time = sk->__sk_common.skc_creator_start_time;
	}

	return check_localhost_skb_to_sock(sk, skb, "conn_request", req);
}


/* called after sbox_sk_clone_security */
static void __maybe_unused sbox_inet_csk_clone(struct sock *newsk, const struct request_sock *req)
{
	DEBUG_NET_LOG("inet_csk_clone tgid from req=%pK to newsk=%pK",
		req, newsk);
	newsk->__sk_common.skc_creator_tgid =
		req->__req_common.skc_creator_tgid;
	newsk->__sk_common.skc_creator_start_time =
		req->__req_common.skc_creator_start_time;
}


/*
 * Ensure that unprivilaged users are restricted to exec in "AT_SECURE" mode
 */
static inline int sbox_bprm_set_creds(struct linux_binprm *bprm)
{
	int is_sec = is_app_profile(current->saved_sandbox_profile_tag) ? 1 : 0;
	bprm->secureexec |= is_sec;
	return 0;
}

static struct security_hook_list sbox_hooks[] = {
	LSM_HOOK_INIT(socket_connect, sbox_socket_connect),
	LSM_HOOK_INIT(socket_sendmsg, sbox_socket_sendmsg),

#ifdef CONFIG_MLSEC_ALLOW_SAME_PROCESS_LOCALHOST
	LSM_HOOK_INIT(socket_bind, sbox_socket_bind),
	LSM_HOOK_INIT(socket_sock_rcv_skb, sbox_socket_sock_rcv_skb),
	LSM_HOOK_INIT(inet_conn_request, sbox_inet_conn_request),
	LSM_HOOK_INIT(inet_csk_clone, sbox_inet_csk_clone),
#endif

	LSM_HOOK_INIT(bprm_set_creds, sbox_bprm_set_creds),
};

static __init int sbox_init(void)
{
	pr_info("sandbox:  Initializing.\n");
	security_add_hooks(sbox_hooks, ARRAY_SIZE(sbox_hooks), "sbox");

#ifdef CONFIG_MLSEC_ALLOW_SAME_PROCESS_LOCALHOST
	nf_register_hook(&local_out_nfho_inet);
	nf_register_hook(&local_out_nfho_inet6);
#endif

	return 0;
}

/* Sandbox requires early initialization for applying security hooks. */
security_initcall(sbox_init);
