#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#define __D3_IMPLEMENTATION__
#include "d3.h"


int main(int parc, char** pars)
{
	int i, c, ok;
	d3* d;
	if (parc < 2)
	{
		printf("Give me a DialogTree binary file as parameter\n");
		return -1;
	}
	d = d3_alloc();
	if (d3_loadfile(d, pars[1]))
	{
		printf("Failed to load %s\n", pars[1]);
		return -1;
	}

	while (1)
	{ 
		printf("%s\n", d->mText);
		printf("-   -  - ---- -  -   -\n");
		for (i = 0; i < d->mAnswerCount; i++)
		{
			printf("%d) %s\n", i, d->mAnswer[i]);
		}
		printf("q) quit\n");
		ok = 0;
		while (!ok)
		{
			c = _getch();
			if (c == 'q')
			{
				return 0;
			}
			c = c - '0';
			if (c < 0 || c > d->mAnswerCount - 1)
			{
				printf("Choice out of range, 0 .. %d expected\n", d->mAnswerCount - 1);
			}
			else
			{
				ok = 1;
				d3_choose(d, c);
			}
		}
	}
	return 0;
}