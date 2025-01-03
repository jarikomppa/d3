/*
DialogTree (d3) engine
Copyright (c) 2020-2025 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#define __D3_IMPLEMENTATION__
#include "d3.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

int main(int parc, char** pars)
{
	int i, c, ok;
	void* s;
	d3* d;
	if (parc < 2)
	{
		printf("Give me a DialogTree binary file as parameter\n");
		return -1;
	}
	s = d3state_alloc();
	d3state_deserialize(s, "d3state.dat");
	d = d3_alloc(s);
	if (d3_loadfile(d, pars[1]))
	{
		printf("Failed to load %s\n", pars[1]);
		return -1;
	}

#ifdef _MSC_VER
	/* enable console "ansi" commands */
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#endif
	while (1)
	{ 
		/* clear screen, move cursor to top*/
		printf("%s", "\033[1J\033[1;1H");
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
				d3_close(d);
				d3state_serialize(s, "d3state.dat");
				d3state_free(s);
				d3_free(d);
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