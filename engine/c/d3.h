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
	void*    mState;
	char*    mData;
	int      mDataAllocated;
	char*    mSyms;
	char*    mMemPool;
	int      mMemPoolSize;
	int      mMemPoolTop;
	int      mCurrentCard;
	int      mRandState;
	int      mGoto;
	int      mGosub;
} d3;

extern d3*   d3_alloc(void *state);
extern void  d3_free(d3* d);
extern int   d3_loadfile(d3* d, char* aFilename);
extern int   d3_usedata(d3* d, char* aData, int len);
extern void  d3_close(d3* d);
extern void  d3_choose(d3* d, int aChoise);

extern void* d3state_alloc();
extern void  d3state_free(void *s);
extern int   d3state_serialize(void *s, char *aFilename);
extern int   d3state_deserialize(void *s, char *aFilename);
extern int   d3state_serialize_size(void *s);
extern int   d3state_serialize_mem(void *s, char *mem, int size);
extern int   d3state_deserialize_mem(void *s, char *mem, int size);

extern void  d3state_set(void *s, char *symbol);
extern void  d3state_clear(void *s, char *symbol);
extern int   d3state_get(void *s, char *symbol);
extern void  d3state_setvalue(void *s, char *symbol, int value);
extern int   d3state_getvalue(void *s, char *symbol);

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

	/* Print value of variable */
	D3_PRINT,

	D3_GT,  /* a>b */
	D3_GTC, /* a>n */
	D3_LT,  /* a<b */
	D3_LTC, /* a<n */
	D3_GTE, /* a>=b */
	D3_GTEC,/* a>=n */
	D3_LTE, /* a<=b */
	D3_LTEC,/* a<=n */
	D3_EQ,  /* a==b */
	D3_EQC, /* a==n */
	D3_IEQ, /* a!=b */
	D3_IEQC,/* a!=n */

	D3_ASSIGN,  /* a=b */
	D3_ASSIGNC, /* a=n */
	D3_ADD,     /* a+b */
	D3_ADDC,    /* a+n */
	D3_SUB,     /* a-b */
	D3_SUBC,    /* a-n */
	D3_MUL,     /* a*b */
	D3_MULC,    /* a*n */
	D3_DIV,     /* a/b */
	D3_DIVC     /* a/n */
};

static int d3i_rand(d3 *d)
{
	d->mRandState = ((d->mRandState * 1103515245U) + 12345U) & 0x7fffffff;
	return d->mRandState;
}

static char * d3i_sym(d3 *d, int id)
{
	/* tag + size + symcount + nsymcount*/
	char *s = d->mSyms + 4 + 4 + 4 + 4;
	/*int symcount = *(int*)(d->mSyms + 4 + 4) + *(int*)(d->mSyms + 4 + 4 + 4);
	if (id >= symcount)
		return 0;*/
	while (id)
	{
		s += *(int*)s + 4;
		id--;
	}
	return s+4;
}

static char * d3i_nsym(d3 *d, int id)
{
	/* tag + size, symcount, nsymcount */
	return d3i_sym(d, id + *(int*)(d->mSyms + 4 + 4));
}

static char * d3i_reserve(d3* d, int aBytes)
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


static int d3i_predicate(d3* d, int *op, int ops)
{
	int pred = 1;
	int i;
	for (i = 0; i < ops; i++)
	{
		switch (*op)
		{
		/*case D3_NOP:*/
		default:
			break;

		case D3_HAS:
			if (!d3state_get(d->mState, d3i_sym(d, op[1])))
				pred = 0;
			break;
		case D3_NOT:
			if (d3state_get(d->mState, d3i_sym(d, op[1])))
				pred = 0;
			break;

		case D3_RND:
			if ((d3i_rand(d) % 100) > op[1])
				pred = 0;
			break;

		case D3_GT:  /* a>b */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) >
				  d3state_getvalue(d->mState, d3i_sym(d, op[2]))))
				pred = 0;
			break;
		case D3_GTC: /* a>n */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) > op[2]))
				pred = 0;
			break;
		case D3_LT:  /* a<b */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) <
				d3state_getvalue(d->mState, d3i_sym(d, op[2]))))
				pred = 0;
			break;
		case D3_LTC: /* a<n */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) < op[2]))
				pred = 0;
			break;
		case D3_GTE: /* a>=b */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) >=
				d3state_getvalue(d->mState, d3i_sym(d, op[2]))))
				pred = 0;
			break;
		case D3_GTEC:/* a>=n */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) >= op[2]))
				pred = 0;
			break;
		case D3_LTE: /* a<=b */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) <=
				d3state_getvalue(d->mState, d3i_sym(d, op[2]))))
				pred = 0;
			break;
		case D3_LTEC:/* a<=n */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) <= op[2]))
				pred = 0;
			break;
		case D3_EQ:  /* a==b */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) ==
				d3state_getvalue(d->mState, d3i_sym(d, op[2]))))
				pred = 0;
			break;
		case D3_EQC: /* a==n */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) == op[2]))
				pred = 0;
			break;
		case D3_IEQ: /* a!=b */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) !=
				d3state_getvalue(d->mState, d3i_sym(d, op[2]))))
				pred = 0;
			break;
		case D3_IEQC:/* a!=n */
			if (!(d3state_getvalue(d->mState, d3i_sym(d, op[1])) != op[2]))
				pred = 0;
			break;
		}
		op += 3;
	}
	return pred;
}

static void d3i_parsecard(d3* d);

static void d3i_execute(d3* d, int *op, int ops, int execute)
{
	int i, t, v;
	for (i = 0; i < ops; i++)
	{
		if (execute || *op == D3_PRINT)
		switch (*op)
		{
		default:
		/* case D3_NOP: */
			break;

		case D3_SET:
			d3state_set(d->mState, d3i_sym(d, op[1]));
			break;
		case D3_CLR:
			d3state_clear(d->mState, d3i_sym(d, op[1]));
			break;
		case D3_XOR:
			if (d3state_get(d->mState, d3i_sym(d, op[1])))
			{
				d3state_clear(d->mState, d3i_sym(d, op[1]));
			}
			else
			{
				d3state_set(d->mState, d3i_sym(d, op[1]));
			}
			break;

		case D3_GO:
			/* Skip reset of text output */
			d->mGoto = 1;
			d->mCurrentCard = op[1];
			d3i_parsecard(d);
			/* Cancel processing of old card */
			d->mGoto = 1;
			return;
			break;
		case D3_GOSUB:
			t = d->mCurrentCard;
			d->mCurrentCard = op[1];
			d->mGosub++;
			d3i_parsecard(d);
			d->mCurrentCard = t;
			break;

		case D3_PRINT:
			t = d3state_getvalue(d->mState, d3i_sym(d, op[1]));
			v = snprintf(NULL, 0, "%d", t);
			snprintf(d3i_reserve(d, v+1), v+1, "%d", t);
			d->mMemPoolTop--; /* overwrite the terminating 0 */
			break;

		case D3_ASSIGN:  /* a=b */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]), d3state_getvalue(d->mState, d3i_sym(d, op[2])));
			break;
		case D3_ASSIGNC: /* a=n */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]), op[2]);
			break;
		case D3_ADD:     /* a+b */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) +
				d3state_getvalue(d->mState, d3i_sym(d, op[2])));
			break;
		case D3_ADDC:    /* a+n */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) +
				op[2]);
			break;
		case D3_SUB:     /* a-b */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) -
				d3state_getvalue(d->mState, d3i_sym(d, op[2])));
			break;
		case D3_SUBC:    /* a-n */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) -
				op[2]);
			break;
		case D3_MUL:     /* a*b */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) *
				d3state_getvalue(d->mState, d3i_sym(d, op[2])));
			break;
		case D3_MULC:    /* a*n */
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) *
				op[2]);
			break;
		case D3_DIV:     /* a/b */
			t = d3state_getvalue(d->mState, d3i_sym(d, op[2]));
			if (t)
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) /
				t);
			break;
		case D3_DIVC:     /* a/n */
			t = op[2];
			if (t)
			d3state_setvalue(d->mState, d3i_sym(d, op[1]),
				d3state_getvalue(d->mState, d3i_sym(d, op[1])) /
				t);
			break;
		}
		op += 3;
	}
}


static void d3i_parsecard(d3* d)
{
	char *p, *op;
	int i;
	int opcount;
	int textlen;
	int pred;
	int target;
	int output;
	
	if (d->mGoto == 0 && d->mGosub == 0)
	{
		d->mMemPoolTop = D3_MAX_ANSWERS * sizeof(char*) + D3_MAX_ANSWERS * sizeof(int);
	}
	d->mGoto = 0;
	d->mAnswerCount = 0;
	p = d->mData + 4 + 4; /* start of first card */
	i = d->mCurrentCard;
	while (i > 0)
	{
		p += *(unsigned int *)(p + 4);
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
		{
			d3i_execute(d, (int*)op, opcount, 1);
		}
		if (d->mGoto)
		{
			d->mGoto = 0;
			return;
		}
		p += 4 + opcount * 4 * 3; /* skip ops */
		textlen = *(unsigned int *)p;
		p += 4; /* text len */
		if (textlen && pred)
		{
			memcpy(d3i_reserve(d, textlen), p, textlen);
		}
		p += textlen;
	}
	memset(d3i_reserve(d, 1), 0, 1); /* zero terminate the q text block */

	if (d->mGosub)
	{
		d->mMemPoolTop--; /* Overwrite the zero terminator */
		d->mGosub--;
		return;
	}

	while (*(unsigned int*)p == D3_SECT)
	{
		p += 4 + 4 + 4; /* tag + size + a */
		target = (int)(p - d->mData); /* offset to data buffer */
		p += 4; /* target */
		/* Set answer text pointer */
		((char**)d->mMemPool)[d->mAnswerCount] = d->mMemPool + d->mMemPoolTop;
		/* Set answer target */
		*(int*)(d->mMemPool + D3_MAX_ANSWERS * sizeof(char*) + d->mAnswerCount * sizeof(int)) = target;
		output = d->mMemPoolTop;
		while (*(unsigned int*)p == D3_PARA)
		{
			p += 4 + 4; /* tag + size */
			opcount = *(unsigned int*)p;
			op = p + 4; /* op count */
			pred = d3i_predicate(d, (int*)op, opcount);
			d3i_execute(d, (int*)op, opcount, 0);
			p += 4 + opcount * 4 * 3; /* skip ops */
			textlen = *(unsigned int*)p;
			p += 4; /* text len */
			if (textlen && pred)
			{
				memcpy(d3i_reserve(d, textlen), p, textlen);
			}
			p += textlen;
		}
		if (output != d->mMemPoolTop)
		{
			/* The answer has text, so we'll include it */
			memset(d3i_reserve(d, 1), 0, 1); /* zero terminate */
			d->mAnswerCount++;
		}
	}

	d->mAnswer = (char**)d->mMemPool;
	d->mText = d->mMemPool + D3_MAX_ANSWERS * sizeof(char*) + D3_MAX_ANSWERS * sizeof(int);	
}

void d3_close(d3* d)
{
	if (d->mData && d->mDataAllocated)
	{
		free(d->mData);
	}
	d->mDataAllocated = 0;
	d->mData = 0;
}

d3* d3_alloc(void *state)
{
	d3* d = (d3*)malloc(sizeof(d3));
	d->mState = state;
	d->mText = 0;
	d->mAnswer = 0;
	d->mAnswerCount = 0;
	d->mData = 0;
	d->mDataAllocated = 1;
	d->mMemPoolSize = 65536;
	d->mMemPool = (char*)malloc(d->mMemPoolSize);
	d->mCurrentCard = 0;
	d->mMemPoolTop = 0;
	d->mSyms = 0;
	d->mGoto = 0;
	d->mGosub = 0;
	return d;
}

void d3_free(d3* d)
{
	d3_close(d);
	free(d->mMemPool);	
	free(d);
}

static int d3i_prep(d3* d, int len)
{
	char* p;
	/* Sanity check: is header tag ok */
	if (*(unsigned int*)d->mData != D3_HEAD)
		return 2;
	/* Sanity check: is file length same as reported */
	if (*(unsigned int*)(d->mData + 4) != len)
		return 3;
	p = d->mData + 4 + 4;
	while (*(unsigned int*)p == D3_CARD)
	{
		p += *(unsigned int*)(p + 4);
	}
	if (*(unsigned int*)p != D3_SYMS)
		return 4;
	d->mSyms = p;
	return 0;
}

int d3_loadfile(d3* d, char* aFilename)
{
	int len;
	int res;
	d3_close(d);
	d->mData = 0;
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
	res = d3i_prep(d, len);
	if (res)
	{
		return res;
	}
	d3i_parsecard(d);
	return 0;
}

int d3_usedata(d3* d, char* aData, int len)
{
	int res;
	if (aData == NULL)
	{
		return 1;
	}	
	d3_close(d);
	d->mData = aData;
	res = d3i_prep(d, len);
	if (res)
	{
		return res;
	}
	d3i_parsecard(d);
	return 0;
}

void d3_choose(d3* d, int aChoise)
{
	char* p = d->mData + *(int*)(d->mMemPool + D3_MAX_ANSWERS * sizeof(char*) + aChoise * 4);
	int id = *(int*)p;
	p += 4 + 4 + 4; /* id, tag, size*/
	int ops = *(int*)p;
	p += 4; /* op count */
	d3i_execute(d, (int*)p, ops, 1);
	d->mCurrentCard = id;
	d3i_parsecard(d);
}

#endif /* __D3_IMPLEMENTATION__ */
#ifdef __cplusplus
}
#endif
#endif /* D3_H_INCLUDED */
