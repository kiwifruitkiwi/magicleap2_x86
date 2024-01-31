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

#include <linux/syscalls.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/module.h>
#include <linux/security.h>
#include <linux/fs_struct.h>
#include <linux/audit.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/binfmts.h>
#include <linux/ml_sec/sandbox.h>
#include <linux/ml_sec/signature_verification.h>
#include <linux/ml_sec/tail_data.h>
#include <linux/ml_sec/common.h>
#include <asm/syscall.h>
#include "sandbox_profile_tags.h"
#include "syscall_inspect.h"
#include "dev/sandbox_dev.h"
#include "syscall_name.h"

/*if true, user application cannot exec files*/
static bool dont_allow_user_exec = false;
module_param(dont_allow_user_exec, bool, 0644);
MODULE_PARM_DESC(dont_allow_user_exec,
	"disable user applications from running external files");

#ifdef SBOX_VERBOSE
static void print_apply_sandbox_msg(struct linux_binprm *bprm,
	const struct sandbox_profile *profile)
{
	char profile_tag_name[5];

	memcpy(profile_tag_name, &profile->profile_tag, 4);
	profile_tag_name[4] = '\0';

	printk(KERN_INFO
	       "sandbox: Applying '%s' sandbox profile on '%s' (%s)...\n",
	       profile_tag_name,
	       (const char *)bprm->file->f_path.dentry->d_name.name,
	       bprm->filename);
}
#endif

static
bool is_permissive_sandbox_profile(const struct sandbox_profile *profile)
{
	if (sbox_dev_is_permissive())
		return true;

	/* TODO: Remove after sandbox is fully enforcing (will be redundant) */
	if (profile != NULL && profile->attributes.permissive_syscalls)
		return true;

	return false;
}

static bool verify_jail(void)
{
	if (current->nsproxy->ipc_ns    == init_nsproxy.ipc_ns ||
	    current->nsproxy->mnt_ns    == init_nsproxy.mnt_ns ||
	    current_user_ns()           == &init_user_ns       ||
	    task_active_pid_ns(current) == &init_pid_ns)
		return false;

	/* TODO: Verify chroot and chdir */
	return true;
}

/*
 * validate profile transition
 * profile transition happens during exec time.
 * to be able to transit from user profile to a new tag this conditions
 * must be met:
 * 1. the progran that runs the exec is USER.
 * 2. the excutable is placed on secure partition, for example "/system"
 * 3. the program being executed is tagged with JALR
 */
static int validate_profile_transition(struct linux_binprm *bprm,
	const struct sandbox_profile *profile)
{
	int rv = 0;
	uint32_t current_profile_tag = current->sandbox_profile ?
		current->sandbox_profile->profile_tag : SBOX_PROFILE_TAG_UNKN;
	uint32_t new_profile_tag = profile ?
		profile->profile_tag : SBOX_PROFILE_TAG_UNKN;

	if (!is_app_profile(current_profile_tag))
		goto out;

	if (!bprm->file || !is_file_on_verified_partition(bprm->file)) {
		pr_err("attempt to run file outside secured partition\n");
		rv = -EPERM;
		goto out;
	}

	if (dont_allow_user_exec) {
		pr_err("user applications are not permitted to run external files\n");
		rv = -EPERM;
		goto out;
	}

	if (new_profile_tag != SBOX_PROFILE_TAG_JALR) {
		pr_err("user application can only run JALR executables\n");
		rv = -EPERM;
		goto out;
	}

out:
	if (IS_ENABLED(CONFIG_MLSEC_SANDBOX_VIOLATION_LOGGING)) {
		if (rv && !sigismember(&current->blocked, SIGVIOLATION) &&
				!fatal_signal_pending(current))
			send_sig_info(SIGVIOLATION, SEND_SIG_FORCED, current);
	}
	return rv;
}


static int apply_sandbox_profile(struct linux_binprm *bprm,
	const struct sandbox_profile *profile)
{
	int rv = 0;
#ifdef SBOX_VERBOSE
	print_apply_sandbox_msg(bprm, profile);
#endif

	if (profile->attributes.enforce_jail && !verify_jail()) {
		bool permissive = sbox_jail_is_permissive();

		sbox_audit_jail(&bprm->file->f_path, permissive);
		if (!permissive)
			return -EPERM;
	}

	if ((rv = validate_profile_transition(bprm, profile)) < 0)
		return rv;

	if (profile->attributes.is_no_new_privs)
		task_set_no_new_privs(current);

	/* Replace the old (parent's) sandbox_profile with the new one. */
	sbox_get_sandbox_profile(profile);
		/* record profile tag transition */
	current->saved_sandbox_profile_tag = current->sandbox_profile ?
		current->sandbox_profile->profile_tag : SBOX_PROFILE_TAG_UNKN;

	sbox_put_profile(current);
	current->sandbox_profile = profile;
	return 0;
}

/*
 * Make sure user hasn't specified any unauthorized taildata params
 */
static int verify_untrusted_taildata(const struct ml_tail_data *tail_data)
{
	/* Verify no sandbox profile other than USER */
	if (tail_data->profile_tag != SBOX_PROFILE_TAG_USER)
		return -1;

	/* Verify no custom SELinux profile */
	if (tail_data->seinfo[0] != 0)
		return -1;

	return 0;
}

/*
 * Find and acquire the Sandbox profile for the given executable file.
 * Note: On success, sbox_put_sandbox_profile() must be called when
 * done using the 'profile'.
 */
static const struct sandbox_profile *acquire_sandbox_profile(
	struct file *file)
{
	uint32_t profile_tag;
	const struct sandbox_profile *profile;

	/* If file is "ML internal" - allow custom sandbox profile,
	   otherwise force USER profile */
	if (sig_check_if_ml_internal(file)) {
		int retval;
		struct ml_tail_data tail_data;

		/* Use runtime profile if present */
		profile = sbox_create_sandbox_profile_from_file(file);
		if (profile)
			goto profile_acquired;

		retval = read_ml_tail_data(file, &tail_data);
		profile_tag = !retval ? tail_data.profile_tag : SBOX_PROFILE_TAG__ML_;
	} else {
		struct ml_tail_data tail_data;

		/* Non-internal file, must verify taildata */
		if (read_ml_tail_data(file, &tail_data) == 0)
			if (verify_untrusted_taildata(&tail_data) != 0) {
				pr_err("sandbox: denied privileged taildata for "
				       "unprivileged file=%s permissive=0\n",
					file->f_path.dentry->d_name.name);

				return ERR_PTR(-EPERM);
			}

		/* Use untrusted sandbox profile */
		profile_tag = SBOX_PROFILE_TAG_USER;

	}

	profile = sbox_lookup_profile(profile_tag);
	if (profile == NULL)
		return ERR_PTR(-EINVAL);

profile_acquired:
	sbox_get_sandbox_profile(profile);
	return profile;
}

bool sbox_is_permissive(void)
{
	return is_permissive_sandbox_profile(current->sandbox_profile);
}

/*
 * allow privilege escalation if the new tag is JALR
 */
bool sbox_is_transition_allowed(const struct linux_binprm *bprm)
{
	bool rv = false;
	const struct sandbox_profile *profile = NULL;

	profile = acquire_sandbox_profile(bprm->file);
	if (IS_ERR(profile)) {
		pr_err("acquire_sandbox_profile() failed\n");
		goto out;
	}

	if (!profile)
		goto out;

	/* allow transition only if the next tag is JALR */
	rv = (profile->profile_tag ==  SBOX_PROFILE_TAG_JALR);
	sbox_put_sandbox_profile(profile);
out:
	return rv;
}
EXPORT_SYMBOL(sbox_is_transition_allowed);

int sbox_apply(struct linux_binprm *bprm)
{
	int retval;
	const struct sandbox_profile *profile;

	profile = acquire_sandbox_profile(bprm->file);
	if (IS_ERR(profile)) {
		pr_err("acquire_sandbox_profile() failed\n");
		return PTR_ERR(profile);
	}

	retval = apply_sandbox_profile(bprm, profile);
	if (retval)
		pr_err("apply_sandbox_profile() failed\n");

	sbox_put_sandbox_profile(profile);
	return retval;
}

static void audit_log_kill(int sys_nr)
{
    const char *sys_name;
	struct task_struct *tsk = current;
	struct audit_buffer *ab;
	sys_name = get_syscall_name(sys_nr);
	if (!sys_name)
		sys_name = "";
	ab = audit_log_start(NULL, GFP_KERNEL, AUDIT_KERNEL);
	if (ab) {
		char comm[sizeof(tsk->comm)];
		audit_log_format(ab, "sandbox:  process killed due to "
		                "syscall=%d (%s) violation", sys_nr, sys_name);
		audit_log_format(ab, " pid=%d comm=", task_pid_nr(tsk));
		audit_log_untrustedstring(ab, get_task_comm(comm, tsk));
		audit_log_end(ab);
	}
}

int sbox_filter_syscall(int sys_nr, unsigned int action)
{
	switch (action) {
	case SANDBOX_ACTION_ALLOW:
		return 0;

	case SANDBOX_ACTION_INSPECT: {
		int err = sbox_inspect_syscall(sys_nr);
		if (!IS_ERR_VALUE((unsigned long)err))
			return 0;

		sbox_audit_syscall(sys_nr, err);
		if (sbox_is_permissive())
			return 0;

		/* On inspection failure, fail the usermode syscall
		   (don't kill the process) */
		syscall_set_return_value(current, task_pt_regs(current), err, 0);
		return -1;
	}

	case SANDBOX_ACTION_DENY:
	default:
		sbox_audit_syscall(sys_nr, 0);
		if (sbox_is_permissive())
			return 0;
		if (IS_ENABLED(CONFIG_MLSEC_SANDBOX_VIOLATION_LOGGING))
			send_sig_info(SIGVIOLATION, SEND_SIG_FORCED, current);
		audit_log_kill(sys_nr);
		do_group_exit(SIGKILL);
	}

	unreachable();
}
