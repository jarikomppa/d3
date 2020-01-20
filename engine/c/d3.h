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
	int      mCurrentCard;
	int      mCurrentSection;
	int      mCurrentParagraph;
} d3;

extern d3* d3_alloc();
extern void d3_free(d3* d);
extern void d3_loadfile(d3* d, char* aFilename);
extern void d3_usedata(d3* d, char* aData);
extern void d3_choose(d3* d, int aChoise);


#ifdef __D3_IMPLEMENTATION__

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

void d3i_parsecard(d3* d)
{
	memcpy(d->mMemPool, "hello\0", 6);
	d->mText = d->mMemPool;
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

void d3_loadfile(d3* d, char* aFilename)
{
	int len;
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	FILE* f;
	fopen_s(&f, aFilename, "rb");
#else
	FILE* f = fopen(aFilename, "rb");
#endif
	if (!f) return;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	d->mData = (char*)malloc(len + 1);
	d->mDataAllocated = 1;
	fread(d->mData, 1, len, f);
	fclose(f);
	d3i_parsecard(d);
}

void d3_usedata(d3* d, char* aData)
{
	d->mData = aData;
	d3i_parsecard(d);
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
