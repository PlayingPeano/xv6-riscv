#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_FIFO_NAME \
	"/Users/gugukukua/Documents/GitHub/xv6-riscv/echo_fifo"
#define DEFAULT_LOG_FILE \
	"/Users/gugukukua/Documents/GitHub/xv6-riscv/echo_server.log"
#define DIAGNOSTIC_INTERVAL_SEC 5
#define BUFFER_SIZE 4096

volatile sig_atomic_t sig_hup = 0;
volatile sig_atomic_t sig_int = 0;
volatile sig_atomic_t sig_alrm = 0;
volatile sig_atomic_t sig_term = 0;
volatile sig_atomic_t sig_usr1 = 0;
volatile sig_atomic_t eof_read = 0;

bool is_daemon = false;
char *fifo_name = NULL;
char *logfile_name = NULL;
int diagnostic_interval = DIAGNOSTIC_INTERVAL_SEC;

FILE *log_fp = NULL;
int log_fd = -1;

int message_count = 0;
int bytes_count = 0;
int alarm_count = 0;

void parse_args(int argc, char **argv);
void signal_log(const char *message, int len);
void signal_handler(int sig);
void set_signal_handlers(void);
void create_fifo(void);
int remove_fifo(void);
void write_log(const char *fmt, ...);
void print_stats(void);
int daemonize(void);
void run_server(void);
int open_fifo(void);
int read_fifo(int);
int close_fifo(int);
void cleanup(void);
void response2signal(void);


int main(int argc, char **argv)
{
	parse_args(argc, argv);
	log_fp = stdout;
	log_fd = STDOUT_FILENO;

	set_signal_handlers();

	if (sig_hup && daemonize() < 0)
	{
		cleanup();
		return EXIT_FAILURE;
	}
	sig_hup = 0;

	create_fifo();

	run_server();
}

void run_server(void)
{
	alarm(diagnostic_interval);
	while (true)
	{
		int fifo_fd = open_fifo();
		if (fifo_fd < 0)
		{
			if (sig_term || sig_int)
			{
				if (sig_term)
				{
					write_log("SIGTERM handled\n");
				}
				if (sig_int)
				{
					write_log("SIGINT handled\n");
				}
				cleanup();
				exit(EXIT_FAILURE);
			}
			perror("Error in run_server: bad open_fifo");
			cleanup();
			exit(EXIT_FAILURE);
		}
		if (read_fifo(fifo_fd) < 0)
		{
			perror("Error in run_server: bad read_fifo");
			cleanup();
			exit(EXIT_FAILURE);
		}
		if (close_fifo(fifo_fd) < 0)
		{
			perror("Error in run_server: bad close_fifo");
			cleanup();
			exit(EXIT_FAILURE);
		}

		if (sig_int)
		{
			write_log("SIGINT handled\n");
			cleanup();
			exit(EXIT_SUCCESS);
		}
		if (sig_term)
		{
			write_log("SIGTERM handled\n");
			cleanup();
			exit(EXIT_SUCCESS);
		}
	}
}

int open_fifo(void)
{
	int fifo_fd;
	while ((fifo_fd = open(fifo_name, O_RDONLY)) < 0)
	{
		if (errno == EINTR)
		{
			if (sig_alrm)
			{
				write_log("Waiting for data...\n");
				++alarm_count;
				sig_alrm = 0;
				alarm(diagnostic_interval);
			}
			else if (sig_usr1)
			{
				print_stats();
				sig_usr1 = 0;
			}
			else if (sig_hup && !is_daemon)
			{
				if (daemonize() < 0)
				{
					perror("Error in open_fifo: bad daemonize");
					return -1;
				}
			}
			else if (sig_int || sig_term)
			{
				return -1;
			}
		}
		else
		{
			perror("Error in open fifo: bad open FIFO");
			return -1;
		}
	}
	return fifo_fd;
}

void response2signal()
{
	if (sig_hup && !is_daemon)
	{
		if (daemonize() < 0)
		{
			perror("Error in response2signal: bad daemonize");
			cleanup();
			exit(EXIT_FAILURE);
		}
	}
	if (sig_int && eof_read)
	{
		write_log("SIGINT: finishing reading... then terminate\n");
		eof_read = 0;
		return;
	}
	if (sig_term)
	{
		write_log("SIGTERM: terminating...\n");
		cleanup();
		exit(EXIT_SUCCESS);
	}
	if (sig_usr1)
	{
		print_stats();
		sig_usr1 = 0;
		return;
	}
	if (sig_alrm)
	{
		write_log("Reading...\n");
		++alarm_count;
		sig_alrm = 0;
		alarm(diagnostic_interval);
		return;
	}
}

int read_fifo(int fifo_fd)
{
	int bytes_read;
	char buff[BUFFER_SIZE + 1];
	bool is_newline_end = true;

	while (true)
	{
		bytes_read = read(fifo_fd, buff, BUFFER_SIZE);

		if (bytes_read < 0)
		{
			if (errno == EINTR)
			{
				response2signal();
				continue;
			}
			perror("Error in read_fifo: bad read FIFO");
			return -1;
		}

		if (!bytes_read)
		{
			if (!is_newline_end)
			{
				write_log("\n");
			}
			++message_count;
			return 0;
		}

		buff[bytes_read] = '\0';
		write_log("%s", buff);
		bytes_count += bytes_read;

		response2signal();
		is_newline_end = (buff[bytes_read - 1] == '\n');
	}
}

int close_fifo(int fifo_fd)

{
	if ((fifo_fd >= 0) && (close(fifo_fd) < 0))
	{
		perror("Error in close_fifo: bad close");
		return -1;
	}
	return 0;
}

void cleanup()
{
	if (log_fp != stdout && log_fp && (fclose(log_fp) < 0))
	{
		perror("Error in cleanup: bad fclose of log_fp");
		exit(EXIT_FAILURE);
	}
	if (remove_fifo() < 0)
	{
		perror("Error in cleanup: bad remove_fifo");
		exit(EXIT_FAILURE);
	}
}

void parse_args(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemonize") == 0))
		{
			sig_hup = true;
		}
		else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fifo") == 0) &&
						 i + 1 < argc)
		{
			fifo_name = argv[++i];
		}
		else if ((strcmp(argv[i], "-l") == 0 ||
							strcmp(argv[i], "--logfile") == 0) &&
						 i + 1 < argc)
		{
			logfile_name = argv[++i];
		}
		else
		{
			fprintf(stderr, "unknown: %s\n", argv[i]);
			fprintf(stderr,
							"usage: %s [-d|--daemonize] [-l|--logfile <logfile>] [-f|--fifo "
							"<fifoname>]\n",
							argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (!fifo_name)
	{
		fifo_name = DEFAULT_FIFO_NAME;
	}
	if (!logfile_name)
	{
		logfile_name = DEFAULT_LOG_FILE;
	}
}

void signal_log(const char *message, int len)
{
	if (log_fd < 0)
	{
		write(STDERR_FILENO, message, len);
	}
	else
	{
		write(log_fd, message, len);
	}
}

void signal_handler(int sig)
{
	const char *sighup_message = "\nSIGHUP received\n";
	const char *sigint_message = "\nSIGINT received\n";
	const char *sigterm_message = "\nSIGTERM received\n";

	if (sig == SIGINT)
	{
		signal_log(sigint_message, strlen(sigint_message));
		sig_int = 1;
		eof_read = 1;
		return;
	}
	if (sig == SIGTERM)
	{
		signal_log(sigterm_message, strlen(sigterm_message));
		sig_term = 1;
		return;
	}
	if (sig == SIGALRM)
	{
		sig_alrm = 1;
		return;
	}
	if (sig == SIGUSR1)
	{
		sig_usr1 = 1;
		return;
	}
	if (sig == SIGHUP)
	{
		signal_log(sighup_message, strlen(sighup_message));
		sig_hup = 1;
		return;
	}
}

void set_signal_handlers(void)
{
	struct sigaction sa = {
		.sa_handler = signal_handler,
		.sa_flags = 0,
	};
	sigemptyset(&sa.sa_mask);
	struct sigaction sa_ignore = {
		.sa_handler = SIG_IGN,
		.sa_flags = 0,
	};
	sigemptyset(&sa_ignore.sa_mask);

	if (sigaction(SIGHUP, &sa, NULL) < 0 || sigaction(SIGINT, &sa, NULL) < 0 ||
			sigaction(SIGTERM, &sa, NULL) < 0 || sigaction(SIGALRM, &sa, NULL) < 0 ||
			sigaction(SIGUSR1, &sa, NULL) < 0 ||
			sigaction(SIGQUIT, &sa_ignore, NULL) < 0)
	{
		perror("Error in set_signal_handlers: bad sigaction");
		cleanup();
		exit(EXIT_FAILURE);
	}
}

void create_fifo(void)
{
	if (mkfifo(fifo_name, 0600) < 0)
	{
		if (errno != EEXIST)
		{
			perror("Error in create_fifo: bad mkfifo");
			cleanup();
			exit(EXIT_FAILURE);
		}
		struct stat st;
		if (stat(fifo_name, &st) < 0)
		{
			perror("Error in create_fifo: bad stat");
			cleanup();
			exit(EXIT_FAILURE);
		}
		if (!S_ISFIFO(st.st_mode))
		{
			fprintf(stderr, "Error in create_fifo: %s exists but is not a FIFO\n",
							fifo_name);
			cleanup();
			exit(EXIT_FAILURE);
		}
	}
}

int remove_fifo(void)
{
	if (unlink(fifo_name) < 0)
	{
		perror("Error in remove_fifo: bad unlink");
		return -1;
	}
	return 0;
}

void write_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(log_fp, fmt, ap);
	fflush(log_fp);
	va_end(ap);
}

void print_stats(void)
{
	write_log("\nStatistics:\n");
	write_log("Messages: %d\n", message_count);
	write_log("Bytes: %d\n", bytes_count);
	write_log("Alarms: %d\n\n", alarm_count);
}

int daemonize(void)
{
	if (is_daemon)
	{
		return 0;
	}

	pid_t pid = fork();

	if (pid < 0)
	{
		perror("Error in daemonize: bad fork");
		return -1;
	}
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	umask(0);

	if (setsid() < 0)
	{
		perror("Error in daemonize: bad setsid");
		return -1;
	}

	pid = fork();
	if (pid < 0)
	{
		perror("Error in daemonize: bad fork");
		return -1;
	}
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	if (chdir("/") < 0)
	{
		perror("Error in daemonize: bad chdir");
		return -1;
	}

	log_fp = fopen(logfile_name, "a");
	if (!log_fp)
	{
		perror("Error in daemonize: bad fopen log");
		return -1;
	}
	log_fd = fileno(log_fp);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	int nullfd = open("/dev/null", O_RDWR);
	if (nullfd < 0)
	{
		fclose(log_fp);
		perror("Error in daemonize: bad open nullfd");
		return -1;
	}

	if (dup2(nullfd, STDIN_FILENO) < 0 || dup2(log_fd, STDOUT_FILENO) < 0 ||
			dup2(log_fd, STDERR_FILENO) < 0)
	{
		fclose(log_fp);
		close(nullfd);
		perror("Error in daemonize: bad dup2");
		return -1;
	}
	close(nullfd);

	is_daemon = true;
	write_log("Daemonized");
	print_stats();
	alarm(diagnostic_interval);
	return 0;
}
