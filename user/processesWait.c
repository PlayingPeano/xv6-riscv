#include "kernel/types.h"
#include "user/user.h"

int 
main(int argc, char* argv[])
{
	int pid = fork();

	if (pid < 0)
	{
		fprintf(2, "Error: can't create a fork\n");
		exit(1);
	}
	else if (pid == 0)
	{
		sleep(100);
		exit(1);
	}
	else
	{
		printf("Parent pid: %d; child pid: %d.\n", getpid(), pid);

		int killMode = 0;
		if (argc > 1 && strcmp(argv[1], "-kill") == 0)
		{
			killMode = 1;
		}

		if (killMode)
		{
			if (kill(pid) < 0)
			{
				fprintf(2, "Error: kill failed for pid %d\n", pid);
				exit(1);
			}
		}

		int childExitCode = 0;
		int waitedPid = wait(&childExitCode);
		if (waitedPid == -1)
		{
			fprintf(2, "Error: something went wrong during waiting\n");
			exit(1);
		}
		
		printf("Exited from process %d with exit code %d\n", waitedPid, childExitCode);
	}

	exit(0);
}