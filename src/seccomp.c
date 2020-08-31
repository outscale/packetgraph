/* Copyright 2019 Outscale SAS
 *
 * This file is part of Packetgraph.
 *
 * Packetgraph is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Packetgraph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Packetgraph.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <packetgraph/common.h>
#include <packetgraph/seccomp-bpf.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

/*
 * * Catch violations so we see, which system call caused the problems
 * *
 */
static void catchViolation(int sig, siginfo_t *si, void *void_context)
{
	fprintf(stderr,
		"Catch sig [%d] when attemption to catch syscall: [%d]\n",
			sig, si->si_syscall);
}
/*
 * * Setup error handling
 * *
 */
static void init_error_handling(void)
{
	struct sigaction sa = { .sa_sigaction = catchViolation,
		.sa_flags = SA_SIGINFO | SA_NODEFER };

	if (sigaction(SIGSYS, &sa, NULL)) {
		printf("Failed to configure SIGSYS handler [%s]\n",
				strerror(errno));
	}
}

int pg_init_seccomp(void)
{
	init_error_handling();

	struct sock_filter filter[] = {
		VALIDATE_ARCHITECTURE,
		EXAMINE_SYSCALL,

		/* List allowed syscalls */
		ALLOW_SYSCALL(exit_group),
		ALLOW_SYSCALL(exit),
		ALLOW_SYSCALL(read),
		ALLOW_SYSCALL(write),
		ALLOW_SYSCALL(open),
		ALLOW_SYSCALL(openat),
		ALLOW_SYSCALL(close),
		ALLOW_SYSCALL(fstat),
		ALLOW_SYSCALL(lstat),
		ALLOW_SYSCALL(lseek),
		ALLOW_SYSCALL(poll),
		ALLOW_SYSCALL(mmap),
		ALLOW_SYSCALL(munmap),
		ALLOW_SYSCALL(brk),
		ALLOW_SYSCALL(rt_sigaction),
		ALLOW_SYSCALL(rt_sigprocmask),
		ALLOW_SYSCALL(ioctl),
		ALLOW_SYSCALL(access),
		ALLOW_SYSCALL(pipe),
		ALLOW_SYSCALL(sched_yield),
		ALLOW_SYSCALL(dup),
		ALLOW_SYSCALL(dup2),
		ALLOW_SYSCALL(nanosleep),
		ALLOW_SYSCALL(socket),
		ALLOW_SYSCALL(sendto),
		ALLOW_SYSCALL(recvmsg),
		ALLOW_SYSCALL(set_mempolicy),
		ALLOW_SYSCALL(recvfrom),
		ALLOW_SYSCALL(bind),
		ALLOW_SYSCALL(listen),
		ALLOW_SYSCALL(getsockname),
		ALLOW_SYSCALL(clone),
		ALLOW_SYSCALL(wait4),
		ALLOW_SYSCALL(kill),
		ALLOW_SYSCALL(fcntl),
		ALLOW_SYSCALL(fsync),
		ALLOW_SYSCALL(ftruncate),
		ALLOW_SYSCALL(getdents),
		ALLOW_SYSCALL(getdents64),
		ALLOW_SYSCALL(rename),
		ALLOW_SYSCALL(unlink),
		ALLOW_SYSCALL(fstatfs),
		ALLOW_SYSCALL(gettid),
		ALLOW_SYSCALL(getpid),
		ALLOW_SYSCALL(futex),
		ALLOW_SYSCALL(sched_setaffinity),
		ALLOW_SYSCALL(tgkill),
		ALLOW_SYSCALL(set_robust_list),
		ALLOW_SYSCALL(fallocate),
		ALLOW_SYSCALL(prctl),
		ALLOW_SYSCALL(move_pages),

		ALLOW_SYSCALL(flock),
		ALLOW_SYSCALL(gettimeofday),
		ALLOW_SYSCALL(stat),
		ALLOW_SYSCALL(clock_gettime),
		ALLOW_SYSCALL(rt_sigreturn),
		ALLOW_SYSCALL(epoll_create),
		ALLOW_SYSCALL(epoll_ctl),
		ALLOW_SYSCALL(epoll_wait),
		ALLOW_SYSCALL(getsockopt),
		ALLOW_SYSCALL(setsockopt),
		ALLOW_SYSCALL(readlink),
		ALLOW_SYSCALL(prlimit64),
		ALLOW_SYSCALL(memfd_create),
		ALLOW_SYSCALL(timerfd_create),
		ALLOW_SYSCALL(uname),
		ALLOW_SYSCALL(iopl),

		TRAP_PROCESS,
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter) / sizeof(*filter)),
		.filter = filter,
	};
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1)
		return -1;
	if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) == -1) {
		fprintf(stderr, "unknown PR_GET_SECCOMP error: %s\n",
				strerror(errno));
		return -1;
	}
	return 0;
}
