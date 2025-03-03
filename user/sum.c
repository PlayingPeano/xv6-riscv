#include "kernel/types.h"
#include "user/user.h"

const int BUFFSIZE = 22;

void ReadToEnd()
{
	char c = '\0';

	while (read(0, &c, 1) > 0 && c != '\n')	{};
}

void Validate(char **buff)
{
	if ((**buff) == '+' || (**buff) == '-')
	{
		++(*buff);
	}
	while ((**buff) != ' ' && (**buff) != '\0')
	{
		if ((**buff) < '0' || (**buff) > '9')
		{
			fprintf(2, "Invalid format\n");
			exit(1);
		}
		++(*buff);
	}
}

int main(int argc, char **argv)
{
	char buff[BUFFSIZE];
	char c = '\0';
	int ind = 0;

	for (;;)
	{
		int readCheck = read(0, &c, 1);
		if (readCheck < 0 || ind + 1 >= BUFFSIZE)
		{
			if (readCheck < 0)
			{
				fprintf(2, "Error while reading\n");
			}
			else
			{
				fprintf(2, "Input is too long\n");
			}
			ReadToEnd();
			exit(1);
		}
		if (readCheck == 0 || c == '\n')
		{
			break;
		}
		buff[ind] = c;
		++ind;
	}
	buff[ind] = '\0';

	char *ptrToBuff = buff;

	Validate(&ptrToBuff);
	if ((*ptrToBuff) != ' ')
	{
		fprintf(2, "Invalid format\n");
		exit(1);
	}
	(*ptrToBuff) = '\0';
	
	++ptrToBuff;
	while((*ptrToBuff) == ' ')
	{
		++ptrToBuff;
	}

	char *secondNumber = ptrToBuff;

	Validate(&ptrToBuff);
	if ((*ptrToBuff) != '\0')
	{
		fprintf(2, "Invalid format\n");
		exit(1);
	}

	int lNum = 0;
	if (buff[0] == '-')
	{
		lNum = -atoi(buff + 1);
	}
	else
	{
		lNum = atoi(buff);
	}
	 
	int rNum = 0;
	if ((*secondNumber) == '-')
	{
		++secondNumber;
		rNum = -atoi(secondNumber);
	}
	else
	{
		rNum = atoi(secondNumber);
	}
	printf("|%d %d|\n", lNum, rNum);
	printf("%d\n", add(lNum, rNum));

	exit(0);
}

