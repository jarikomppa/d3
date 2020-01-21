/*
DialogTree (d3) engine
Copyright (c) 2020 Jari Komppa
http://iki.fi/sol
Released under Unlicense

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

This is a single file header-only library.

Usage:
In *one* source file, define __D3_IMPLEMENTATION__ before including this file:

#define __D3_IMPLEMENTATION__
#include "d3.h"

In others, including the d3.h will do.
*/

#ifndef D3_H_INCLUDED
#define D3_H_INCLUDED

#define D3_MAX_ANSWERS 16

#ifdef __cplusplus
extern "C" {
#endif

typedef struct d3_ {
	char*    mText;
	char**   mAnswer;
	int      mAnswerCount;
	/* Internals */
	char*    mData;
	int      mDataAllocated;
	char*    mMemPool;
	int      mMemPoolSize;
	int      mMemPoolTop;
	int      mCurrentCard;
	int      mCurrentSection;
	int      mCurrentParagraph;
} d3;

extern d3* d3_alloc();
extern void d3_free(d3* d);
extern int d3_loadfile(d3* d, char* aFilename);
extern int d3_usedata(d3* d, char* aData, int len);
extern void d3_choose(d3* d, int aChoise);

#ifdef __D3_IMPLEMENTATION__

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

enum d3_tags
{
	D3_HEAD = 0x30303344,
	D3_CARD = 0x44524143,
	D3_SECT = 0x54434553,
	D3_PARA = 0x41524150,
	D3_SYMS = 0x534d5953
};

enum d3_opcodeval
{
	D3_NOP,

	D3_HAS,
	D3_NOT,

	D3_SET,
	D3_CLR,
	D3_XOR,

	D3_RND,

	D3_GO,
	D3_GOSUB,

	// Print value of variable
	D3_PRINT,

	// variable-constant pairs. The code below depends on the pairing.
	D3_GT,  // a>b
	D3_GTC, // a>n
	D3_LT,  // a<b
	D3_LTC, // a<n
	D3_GTE, // a>=b
	D3_GTEC,// a>=n
	D3_LTE, // a<=b
	D3_LTEC,// a<=n
	D3_EQ,  // a==b
	D3_EQC, // a==n
	D3_IEQ, // a!=b
	D3_IEQC,// a!=n

	D3_ASSIGN,  // a=b
	D3_ASSIGNC, // a=n
	D3_ADD,     // a+b
	D3_ADDC,    // a+n
	D3_SUB,     // a-b
	D3_SUBC,    // a-n
	D3_MUL,     // a*b
	D3_MULC,    // a*n
	D3_DIV,     // a/b
	D3_DIVC     // a/n
};

char * d3i_reserve(d3* d, int aBytes)
{
	char* b;
	int newsize;
	int i;
	if (d->mMemPoolTop + aBytes > d->mMemPoolSize)
	{
		newsize = d->mMemPoolSize;
		while (d->mMemPoolTop + aBytes > newsize)
		{
			newsize *= 2;
		}
		b = (char*)malloc(newsize);
		memcpy(b, d->mMemPool, d->mMemPoolSize);
		free(d->mMemPool);
		/* Readjust pointers */
		for (i = 0; i < D3_MAX_ANSWERS; i++)
		{
			((char**)d->mMemPool)[i] += b - d->mMemPool;
		}

		d->mMemPool = b;
		d->mMemPoolSize = newsize;
	}
	b = d->mMemPool + d->mMemPoolTop;
	d->mMemPoolTop += aBytes;
	return b;
}


int d3i_predicate(d3* d, int *op, int ops)
{
	int pred = 1;
	int i;
	for (i = 0; i < ops; i++)
	{
		switch (*op)
		{
		case D3_NOP:
			break;

		case D3_HAS:
			break;
		case D3_NOT:
			break;

		case D3_SET:
			break;
		case D3_CLR:
			break;
		case D3_XOR:
			break;

		case D3_RND:
			break;

		case D3_GO:
			break;
		case D3_GOSUB:
			break;

		case D3_PRINT:
			break;

		case D3_GT:  // a>b
			break;
		case D3_GTC: // a>n
			break;
		case D3_LT:  // a<b
			break;
		case D3_LTC: // a<n
			break;
		case D3_GTE: // a>=b
			break;
		case D3_GTEC:// a>=n
			break;
		case D3_LTE: // a<=b
			break;
		case D3_LTEC:// a<=n
			break;
		case D3_EQ:  // a==b
			break;
		case D3_EQC: // a==n
			break;
		case D3_IEQ: // a!=b
			break;
		case D3_IEQC:// a!=n
			break;

		case D3_ASSIGN:  // a=b
			break;
		case D3_ASSIGNC: // a=n
			break;
		case D3_ADD:     // a+b
			break;
		case D3_ADDC:    // a+n
			break;
		case D3_SUB:     // a-b
			break;
		case D3_SUBC:    // a-n
			break;
		case D3_MUL:     // a*b
			break;
		case D3_MULC:    // a*n
			break;
		case D3_DIV:     // a/b
			break;
		case D3_DIVC:     // a/n
			break;
		}
		op += 3;
	}
	return pred;
}

void d3i_execute(d3* d, int *op, int ops)
{
	int i;
	int v;
	for (i = 0; i < ops; i++)
	{
		switch (*op)
		{
		case D3_NOP:
			break;

		case D3_HAS:
			break;
		case D3_NOT:
			break;

		case D3_SET:
			break;
		case D3_CLR:
			break;
		case D3_XOR:
			break;

		case D3_RND:
			break;

		case D3_GO:
			break;
		case D3_GOSUB:
			break;

		case D3_PRINT:
			v = snprintf(NULL, 0, "%d", 0x7447);
			snprintf(d3i_reserve(d, v+1), v+1, "%d", 0x7447);
			d->mMemPoolTop--; /* overwrite the terminating 0 */
			break;

		case D3_GT:  // a>b
			break;
		case D3_GTC: // a>n
			break;
		case D3_LT:  // a<b
			break;
		case D3_LTC: // a<n
			break;
		case D3_GTE: // a>=b
			break;
		case D3_GTEC:// a>=n
			break;
		case D3_LTE: // a<=b
			break;
		case D3_LTEC:// a<=n
			break;
		case D3_EQ:  // a==b
			break;
		case D3_EQC: // a==n
			break;
		case D3_IEQ: // a!=b
			break;
		case D3_IEQC:// a!=n
			break;

		case D3_ASSIGN:  // a=b
			break;
		case D3_ASSIGNC: // a=n
			break;
		case D3_ADD:     // a+b
			break;
		case D3_ADDC:    // a+n
			break;
		case D3_SUB:     // a-b
			break;
		case D3_SUBC:    // a-n
			break;
		case D3_MUL:     // a*b
			break;
		case D3_MULC:    // a*n
			break;
		case D3_DIV:     // a/b
			break;
		case D3_DIVC:     // a/n
			break;
		}
		op += 3;
	}
}


void d3i_parsecard(d3* d)
{
	char *p, *op;
	int i;
	int opcount;
	int textlen;
	int pred;
	d->mMemPoolTop = D3_MAX_ANSWERS * sizeof(char*) + D3_MAX_ANSWERS * sizeof(int);
	d->mAnswerCount = 0;
//	((char**)d->mMemPool)[0] = d3i_reserve(d, 6);
	p = d->mData + 8; /* start of first card */
	d->mCurrentCard = 1; // HAX
	i = d->mCurrentCard;
	while (i > 0)
	{
		p += *(unsigned int *)(p + 4)+4;
		i--;
	}

	/* card + size + sect + size + id + q -> para */
	p += 4 + 4 + 4 + 4 + 4 + 4;
	while (*(unsigned int *)p == D3_PARA)
	{
		p += 4 + 4; /* tag + size */
		opcount = *(unsigned int *)p;
		op = p + 4; /* op count */
		pred = d3i_predicate(d, (int*)op, opcount);
		if (pred)
			d3i_execute(d, (int*)op, opcount);
		p += 4 + opcount * 4 * 3; /* skip ops */
		textlen = *(unsigned int *)p;
		p += 4; /* text len */
		if (textlen && pred)
			memcpy(d3i_reserve(d, textlen), p, textlen);
		p += textlen;
	}
	memset(d3i_reserve(d, 1), 0, 1);
	d->mText = d->mMemPool + D3_MAX_ANSWERS * sizeof(char*) + D3_MAX_ANSWERS * sizeof(int);
	
}


d3* d3_alloc()
{
	d3* d = (d3*)malloc(sizeof(d3));
	d->mText = 0;
	d->mAnswer = 0;
	d->mAnswerCount = 0;
	d->mData = 0;
	d->mDataAllocated = 1;
	d->mMemPoolSize = 65536;
	d->mMemPool = (char*)malloc(d->mMemPoolSize);
	d->mCurrentCard = 0;
	d->mCurrentSection = 0;
	d->mCurrentParagraph = 0;
	d->mMemPoolTop = 0;
	return d;
}

void d3_free(d3* d)
{
	if (d->mDataAllocated)
	{
		free(d->mData);
	}
	free(d->mMemPool);	
	free(d);
}

int d3_loadfile(d3* d, char* aFilename)
{
	int len;
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	FILE* f;
	fopen_s(&f, aFilename, "rb");
#else
	FILE* f = fopen(aFilename, "rb");
#endif
	if (!f) return 1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	d->mData = (char*)malloc(len + 1);
	d->mDataAllocated = 1;
	fread(d->mData, 1, len, f);
	fclose(f);
	// Sanity check: is header tag ok
	if (*(unsigned int*)d->mData != D3_HEAD)
		return 2;
	// Sanity check: is file length same as reported
	if (*(unsigned int*)(d->mData + 4) != len - 4)
		return 3;

	d3i_parsecard(d);
	return 0;
}

int d3_usedata(d3* d, char* aData, int len)
{
	if (aData == NULL)
		return 1;
	if (d->mDataAllocated)
		free(d->mData);
	d->mDataAllocated = 0;
	d->mData = aData;
	// Sanity check: is header tag ok
	if (*(unsigned int*)d->mData != D3_HEAD)
		return 2;
	// Sanity check: is file length same as reported
	if (*(unsigned int*)(d->mData + 4) != len - 4)
		return 3;
	d3i_parsecard(d);
	return 0;
}

void d3_choose(d3* d, int aChoise)
{
	d3i_parsecard(d);
}

#endif /* __D3_IMPLEMENTATION__ */
#ifdef __cplusplus
}
#endif
#endif /* D3_H_INCLUDED */
