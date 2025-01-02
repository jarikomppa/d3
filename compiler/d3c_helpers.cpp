/*
DialogTree (d3) engine compiler
Copyright (c) 2020-2025 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"

// delete[]able string duplicate
char* mystrdup(const char* src)
{
	char* t;
	int l = strlen(src);
	t = new char[l + 1];
	memcpy(t, src, l);
	t[l] = 0;
	return t;
}

int is_whitespace(char aCharacter)
{
	switch (aCharacter)
	{
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return 1;
	}
	return 0;
}

void store_cmd(int aOperation, int aParameter1, int aParameter2)
{
	if (gVerbose)
	{
		printf("    Opcode: ");
		disasm_op(stdout, aOperation, aParameter1, aParameter2);
		printf("\n");
	}
	Op* o = new Op();
	o->mOpcode = aOperation;
	o->mOperand1 = aParameter1;
	o->mOperand2 = aParameter2;
	if (gCurrentOp == 0)
	{
		gCurrentParagraph->mOp = o;
	}
	else
	{
		gCurrentOp->mNext = o;
	}
	gCurrentOp = o;
}

void store_cmd(int aOperation, int aParameter)
{
	store_cmd(aOperation, aParameter, 0);
}


int is_numeric(char* aString)
{
	if (!*aString) return 0;
	while (*aString)
	{
		if (*aString < '0' || *aString > '9')
			return 0;
		aString++;
	}
	return 1;
}

int is_oksymbol(char* aString)
{
	if (aString == 0 || *aString == 0)
		return 0;
	while (*aString)
	{
		int ok = 0;
		if (*aString >= 'a' && *aString <= 'z') ok = 1;
		if (*aString >= 'A' && *aString <= 'Z') ok = 1;
		if (*aString >= '0' && *aString <= '9') ok = 1;
		if (*aString == '_') ok = 1;
		if (!ok)
			return 0;
		aString++;
	}
	return 1;
}

int is_okglobalsymbol(char* aString)
{
	if (aString == 0 || *aString == 0)
		return 0;
	while (*aString)
	{
		int ok = 0;
		if (*aString >= 'a' && *aString <= 'z') ok = 1;
		if (*aString >= 'A' && *aString <= 'Z') ok = 1;
		if (*aString >= '0' && *aString <= '9') ok = 1;
		if (*aString == '_' || *aString == '.') ok = 1;
		if (!ok)
			return 0;
		aString++;
	}
	return 1;
}

int is_globalsymbol(char* aString)
{
	if (aString == 0 || *aString == 0)
		return 0;

	while (*aString)
	{
		if (*aString == '.') return 1;
		aString++;
	}
	return 0;
}
