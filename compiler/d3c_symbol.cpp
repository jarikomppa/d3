/*
DialogTree (d3) engine compiler
Copyright (c) 2020-2025 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"

Symbol::Symbol()
{
	mCount = 0;
	mFirstIndex = 0;
	int i;
	for (i = 0; i < MAX_SYMBOLS * 2; i++)
	{
		mName[i] = 0;
		mHits[i] = 0;
		mHash[i] = 0;
	}
}

int Symbol::calcHash(char* aString)
{
	unsigned int i = 0;
	while (*aString)
	{
		char c = toupper(*aString);

		i = (i << 11) | (i >> 21);
		i ^= *aString;
		aString++;
	}

	return 0;
}

int Symbol::getId(char* aString)
{
	int i;
	char tempSym[64];
	if (gPrefix == 0 || is_globalsymbol(aString))
	{
		snprintf(tempSym, 64, "%s", aString);
	}
	else
	{
		snprintf(tempSym, 64, "%s.%s", gPrefix, aString);
	}

	int hash = calcHash(tempSym);
	for (i = 0; i < mCount; i++)
	{
		if (mHash[i] == hash && stricmp(mName[i], tempSym) == 0)
		{
			mHits[i]++;
			return i + mFirstIndex;
		}
	}
	if (mCount >= MAX_SYMBOLS)
	{
		printf("Too many symbols %d\n", MAX_SYMBOLS);
		exit(-1);
	}
	if (!is_okglobalsymbol(aString))
	{
		printf("Warning: symbol '%s' uses bad characters, please use a-z,A-Z,0-9,_\n", aString);
	}
	mName[mCount] = mystrdup(tempSym);
	mHits[mCount] = 1;
	mHash[mCount] = hash;
	mCount++;
	return mCount - 1;
}

int Symbol::isSymbol(char* aString)
{
	int i;
	char tempSym[64];
	if (gPrefix == 0 || is_globalsymbol(aString))
	{
		snprintf(tempSym, 64, "%s", aString);
	}
	else
	{
		snprintf(tempSym, 64, "%s.%s", gPrefix, aString);
	}

	int hash = calcHash(tempSym);
	for (i = 0; i < mCount; i++)
	{
		if (mHash[i] == hash && stricmp(mName[i], tempSym) == 0)
		{
			return 1;
		}
	}
	return 0;
}
