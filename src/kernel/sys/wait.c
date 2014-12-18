/*
 * Copyright (C) 2011-2013 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * <sys/wait.c> - wait() system call implementation.
 */

#include <nanvix/const.h>
#include <nanvix/mm.h>
#include <nanvix/pm.h>
#include <sys/types.h>
#include <errno.h>

/* Sleeping chain. */
PRIVATE struct process *chain = NULL;

/*
 * Waits for a child process to terminate.
 */
PUBLIC pid_t sys_wait(int *stat_loc)
{
	int sig;
	pid_t pid;
	struct process *p;

	/* Has no permissions to write at stat_loc. */
	if ((stat_loc != NULL) && (!chkmem(stat_loc, sizeof(int), MAY_WRITE)))
		return (-EINVAL);

repeat:

	/* Nobody to wait for. */
	if (curr_proc->nchildren == 0)
		return (-ECHILD);

	/* Look for child processes. */
	for (p = FIRST_PROC; p <= LAST_PROC; p++)
	{
		/* Skip invalid processes. */
		if (!IS_VALID(p))
			continue;
			
		 /* Found. */
		if (p->father == curr_proc)
		{
			/* Task has already terminated. */
			if (p->state == PROC_ZOMBIE)
			{
				/* Get exit code. */
				if (stat_loc != NULL)
					*stat_loc = p->status;
				
				/* 
				 * Get information from child
				 * process before burying it.
				 */
				pid = p->pid;
				curr_proc->cutime += p->utime;
				curr_proc->cktime += p->ktime;

				/* Bury child process. */
				bury(p);
				
				return (pid);
			}
		}
	}

	sleep(&chain, PRIO_USER);
	sig = issig();
	
	/* Go back and check what happened. */
	if ((sig == SIGNULL) || (sig == SIGCHLD))
		goto repeat;
		
	return (-EINTR);
}
