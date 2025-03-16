#include "kernel/types.h"
#include "user/user.h"

const int MAXARGS = 32;
const int PAGESIZE = 4096;
const int BUFFSIZE = 100;

int 
main(int argc, char* argv[])
{
	if (argc < 2 || argc >= MAXARGS)
	{
		fprintf(2, "Error: argc must be in [2, 32]\n");
		exit(1);
	}

	for (int i = 1; i < argc; ++i)
	{
        if (strlen(argv[i]) >= PAGESIZE)
		{
            fprintf(2, "Error: arg %d exceeds page size\n", i);
            exit(1);
        }
    }

	int pipefd[2];
    if (pipe(pipefd) == -1) 
	{
        fprintf(2, "Error: Couldn't make a pipe\n");
		exit(1);
    }
	
	int pid = fork();
    if (pid == -1) 
	{
        fprintf(2, "Error: Couldn't make a fork\n");
		exit(1);
    }

    if (pid == 0) 
	{
		if (close(0)) 
		{
            fprintf(2, "Error: Couldn't close stdin\n");
			exit(1);
        }

		if (dup(pipefd[0]) == -1)
		{
            fprintf(2, "Error: can't make a dup\n");
			exit(1);
        }
		
        if (close(pipefd[0])) 
		{
			fprintf(2, "Error: Can't close read end of pipe\n");
			exit(1);
        }
		
		if (close(pipefd[1])) 
		{
			fprintf(2, "Error: Couldn't close write end of pipe\n");
			exit(1);
		}
		
        char *argv[] = {"/wc", 0};
		exec("/wc", argv);
        
		fprintf(2, "Error: Can't do exec\n");
		exit(1);
    } 
	else if (pid > 0)
	{
        if (close(pipefd[0])) 
		{
            fprintf(2, "Error: Can't close read end of pipe\n");
			exit(1);
        }

		char buff[BUFFSIZE];
		int idx = 0;
        for (int i = 1; i < argc; ++i) 
		{
            int len = strlen(argv[i]);
			for (int j = 0; j < len; ++j)
			{
				if (idx >= BUFFSIZE - 1) //to handle a possibility of idx = BUFFSIZE + 1 when adding '\n' at the end.
				{
					int written = 0;
					while (written < idx)
					{
						int n = write(pipefd[1], buff + written, idx - written);
                        if (n == -1) 
						{
                            fprintf(2, "Error: Can't write\n");
                            exit(1);
                        }
                        written += n;
					}
					idx = 0;
				}
				buff[idx++] = argv[i][j];
			}
			buff[idx++] = '\n';
        }

		if (idx > 0)
		{
			int written = 0;
			while (written < idx)
			{
				int n = write(pipefd[1], buff + written, idx - written);
        	    if (n == -1) 
				{
        	        fprintf(2, "Error: Can't write\n");
        	        exit(1);
        	    }
        	    written += n;
			}
		}

        if (close(pipefd[1])) 
		{
            fprintf(2, "Error: Can't close write end of pipe\n");
			exit(1);
        }

		int status;
		if (wait(&status) < 1)
		{
			fprintf(2, "Error: problem occured while waiting\n");
			exit(1);
		}

		if (status != 0)
		{
			fprintf(2, "Error: process ended with status %d\n", status);
			exit(1);
		}
    }
	else
	{
		fprintf(2, "Error: couldn't make a fork\n");
		exit(1);
	}

	exit(0);
}