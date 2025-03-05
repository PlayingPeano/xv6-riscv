#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

const int BUFF_SIZE = 4096;

int main(int argc, char *argv[]) 
{
    int pipefd[2];
    if (pipe(pipefd) < 0) 
	{
        perror("Error: pipe error\n");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) 
	{
		close(pipefd[0]);
		close(pipefd[1]);
        perror("Error: fork error\n");
        return EXIT_FAILURE;
    }

    if (pid == 0) 
	{
        if (close(pipefd[1]) < 0)
		{
			perror("Error: output closing error\n");
			close(pipefd[0]);
			return EXIT_FAILURE;
		}

        // char buff[BUFF_SIZE];
        // ssize_t bytes = 0;
        // while ((bytes = read(pipefd[0], buff, BUFF_SIZE)) > 0) 
		// {
        //     if (write(STDOUT_FILENO, buff, bytes) < 0)
        //     {
        //         perror("Error: occured while writing\n");
        //         close(pipefd[0]);
        //         return EXIT_FAILURE;
        //     }
        // }

        char buff[BUFF_SIZE];
        ssize_t bytes = 0;
        while ((bytes = read(pipefd[0], buff, BUFF_SIZE)) > 0) 
        {
            ssize_t written = 0;
            while (written < bytes) 
            {
                ssize_t n = write(STDOUT_FILENO, buff + written, bytes - written);
                if (n <= 0) 
                {
                    perror("Error: occurred while writing\n");
                    close(pipefd[0]);
                    return EXIT_FAILURE;
                }
                written += n;
            }
        }
        
        if (bytes < 0)
        {
            perror("Error: occured while reading\n");
            close(pipefd[0]);
            return EXIT_FAILURE;
        }

        if (close(pipefd[0]) < 0)
        {
            perror("Error: input closing error\n");
            return EXIT_FAILURE;
        }
    } else 
	{
        if (close(pipefd[0]) < 0)
        {
            perror("Error: input closing error\n");
            kill(pid, SIGKILL);
            return EXIT_FAILURE;
        }

        for (int i = 1; i < argc; ++i) 
		{
            size_t len = strlen(argv[i]);
            ssize_t written = 0;
            while (written < len) 
			{
                ssize_t n = write(pipefd[1], argv[i] + written, len - written);
                if (n < 0) 
				{
                    perror("Error: occured on writing\n");
                    kill(pid, SIGKILL);
                    close(pipefd[1]);
                    return EXIT_FAILURE;
                }
                written += n;
            }
            if (write(pipefd[1], "\n", 1) < 0)
            {
                perror("Error: occured on writing\n");
                kill(pid, SIGKILL);
                close(pipefd[1]);
                return EXIT_FAILURE;
            }
        }

        if (close(pipefd[1]) < 0)
        {
            perror("Error: output closing error\n");
            kill(pid, SIGKILL);
            return EXIT_FAILURE;
        }

        int status = 0;
        if (wait(&status) < 0)
        {
            perror("Error: occured while waiting\n");
            return EXIT_FAILURE;
        }
        
        if (!WIFEXITED(status) || WEXITSTATUS(status))
        {
            return EXIT_FAILURE;
        }
    }

	return EXIT_SUCCESS;
}