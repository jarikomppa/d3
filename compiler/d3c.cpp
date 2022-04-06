/*
DialogTree (d3) engine compiler
Copyright (c) 2020 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"


void report()
{
	int i;
	printf("\n");
	printf("Token Hits Symbol\n");
	for (i = 0; i < gSymbol.mCount; i++)
	{
		printf("%5d %4d \"%s\"%s\n", i, gSymbol.mHits[i], gSymbol.mName[i], gSymbol.mHits[i] < 2 ? " <- Warning" : "");
	}

	if (gNumber.mCount)
	{
		printf("\n");
		printf("Token Hits Number\n");
		for (i = 0; i < gNumber.mCount; i++)
		{
			printf("%5d %4d \"%s\"%s\n", i, gNumber.mHits[i], gNumber.mName[i], gNumber.mHits[i] < 2 ? " <- Warning" : "");
		}
	}
}


void sanity_check()
{
	int i, j;
	for (i = 0; i < gSymbol.mCount; i++)
	{
		for (j = 0; j < gNumber.mCount; j++)
		{
			if (stricmp(gSymbol.mName[i], gNumber.mName[j]) == 0)
			{
				printf("Warning: Symbol \"%s\" used both as a flag and a number. This probably isn't what you wanted.\n", gSymbol.mName[i]);
			}
		}
	}

	if (gSymbol.mCount > MAX_SYMBOLS)
	{
		printf("Too many symbols in use (%d > %d)\n", gSymbol.mCount, MAX_SYMBOLS);
		exit(-1);
	}

	if (gNumber.mCount > MAX_NUMBERS)
	{
		printf("Too many numeric variables in use (%d > %d)\n", gNumber.mCount, MAX_NUMBERS);
		exit(-1);
	}
}


int main(int aParc, char** aPars)
{
	printf("DialogTree compiler, by Jari Komppa http://iki.fi/sol/\n");
	if (aParc < 3)
	{
		printf(
			"%s <input> <output> [flags]\n"
			"Optional flags:\n"
			"  -v   verbose (useful for debugging)\n"
			"  -q   quiet (minimal output)\n"
			"  -m   disable implicit flags by default\n"
			"  -t   output data tree file for debugging (.tree)\n"
			"  -d   Output graphviz .dot file for debugging (.dot)\n"
			"  -j   JSON output\n"
			"  -b   Binary output (default)\n",
			aPars[0]);
		return -1;
	}
	int infile = -1;
	int outfile = -1;
	int i;
	for (i = 1; i < aParc; i++)
	{
		if (aPars[i][0] == '-')
		{
			switch (aPars[i][1])
			{
			case 'v':
			case 'V':
				gQuiet = false;
				gVerbose = true;
				break;
			case 'q':
			case 'Q':
				gQuiet = true;
				gVerbose = false;
				break;
			case 't':
			case 'T':
				gDumpTree = true;
				break;
			case 'd':
			case 'D':
				gDumpDot = true;
				break;
			case 'j':
			case 'J':
				gJsonOutput = true;
				break;
			case 'b':
			case 'B':
				gJsonOutput = false;
				break;
			case 'm':
			case 'M':
				gImplicitFlags = false;
				break;
			default:
				printf("Unknown parameter \"%s\"\n", aPars[i]);
				exit(-1);
			}
		}
		else
		{
			if (infile == -1)
			{
				infile = i;
			}
			else
			{
				if (outfile == -1)
				{
					outfile = i;
				}
				else
				{
					printf("Invalid parameter \"%s\" (input and output files already defined)\n", aPars[i]);
					exit(-1);
				}
			}
		}
	}

	if (outfile == -1)
	{
		printf("Invalid parameters (run without params for help)\n");
		exit(-1);
	}

	scan_first_pass(aPars[infile]);

	scan_second_pass(aPars[infile]);

	patch_tree_pass();

	if (gDumpTree)
	{
		dump_tree(aPars[outfile]);
	}

	if (gDumpDot)
	{
		dump_dot(aPars[outfile]);
	}

	if (!gQuiet) report();

	sanity_check();

	output(aPars[outfile]);

	return 0;
}
