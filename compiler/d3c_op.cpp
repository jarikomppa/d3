/*
DialogTree (d3) engine compiler
Copyright (c) 2020-2025 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"

bool is_predicate(int aOp)
{
	switch (aOp)
	{
	case OP_HAS:
	case OP_NOT:
		//case OP_RND: // technically a predicate, but not good for what we use this function for
	case OP_GT:
	case OP_GTC:
	case OP_LT:
	case OP_LTC:
	case OP_GTE:
	case OP_GTEC:
	case OP_LTE:
	case OP_LTEC:
	case OP_EQ:
	case OP_EQC:
	case OP_IEQ:
	case OP_IEQC:
		return true;
	}
	return false;
}

void disasm_op(FILE* f, int aOperation, int aParameter1, int aParameter2)
{
	switch (aOperation)
	{
	case OP_NOP:     fprintf(f, "NOP"); break;
	case OP_HAS:     fprintf(f, "HAS(%s)", gSymbol.mName[aParameter1]); break;
	case OP_NOT:     fprintf(f, "NOT(%s)", gSymbol.mName[aParameter1]); break;
	case OP_SET:     fprintf(f, "SET(%s)", gSymbol.mName[aParameter1]); break;
	case OP_CLR:     fprintf(f, "CLR(%s)", gSymbol.mName[aParameter1]); break;
	case OP_XOR:     fprintf(f, "XOR(%s)", gSymbol.mName[aParameter1]); break;
	case OP_RND:     fprintf(f, "RND(%d)", aParameter1); break;
	case OP_GO:      fprintf(f, "GO(%s)", gSymbol.mName[aParameter1]); break;
	case OP_GOSUB:   fprintf(f, "GOSUB(%s)", gSymbol.mName[aParameter1]); break;
	case OP_PRINT:   fprintf(f, "PRINT(%s)", gNumber.mName[aParameter1]); break;
	case OP_GT:      fprintf(f, "GT(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_GTC:     fprintf(f, "GTC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_LT:      fprintf(f, "LT(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_LTC:     fprintf(f, "LTC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_GTE:     fprintf(f, "GTE(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_GTEC:    fprintf(f, "GTEC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_LTE:     fprintf(f, "LTE(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_LTEC:    fprintf(f, "LTEC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_EQ:      fprintf(f, "EQ(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_EQC:     fprintf(f, "EQC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_IEQ:     fprintf(f, "IEQ(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_IEQC:    fprintf(f, "IEQC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_ASSIGN:  fprintf(f, "ASSIGN(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_ASSIGNC: fprintf(f, "ASSIGNC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_ADD:     fprintf(f, "ADD(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_ADDC:    fprintf(f, "ADDC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_SUB:     fprintf(f, "SUB(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_SUBC:    fprintf(f, "SUBC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_MUL:     fprintf(f, "MUL(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_MULC:    fprintf(f, "MULC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_DIV:     fprintf(f, "DIV(%s,%s)", gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_DIVC:    fprintf(f, "DIVC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	default:
		fprintf(f, "Unknown operation %d (%d, %d)", aOperation, aParameter1, aParameter2);
	}
}


void set_op(int aOpCode, int aValue)
{
	if (aValue > 255)
	{
		printf("Parameter value out of range, %s line %d\n", gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
		exit(-1);
	}

	store_cmd(aOpCode, aValue);
}

void set_number_op(int aOpCode, int aValue)
{
	store_cmd(aOpCode, aValue);
}

void set_number_op(int aOpCode, int aValue1, int aValue2)
{
	store_cmd(aOpCode, aValue1, aValue2);
}

void set_go_op(int aOperation, int aValue)
{
	if (aValue >= gMaxCardSymbol)
	{
		printf("Invalid %s parameter: symbol \"%s\" is not a card, %s line %d\n",
			aOperation == OP_GOSUB ? "GOSUB" : "GO",
			gSymbol.mName[aValue],
			gFileStack[gFile].mFilename,
			gFileStack[gFile].mLine);
		exit(-1);
	}
	set_op(aOperation, aValue);
}

void parse_op(char* aOperation)
{
	// Op may be of form "foo" "!foo" or "foo:bar" or "foo[intop]bar" where intop is <,>,<=,>=,==,!=,=,+,-,/,*
	if (aOperation[0] == 0)
	{
		printf("Syntax error (op=null), %s line %d\n", gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
		exit(-1);
	}
	if (aOperation[0] == ':' ||
		aOperation[0] == '>' ||
		aOperation[0] == '<' ||
		aOperation[0] == '=' ||
		aOperation[0] == '+' ||
		aOperation[0] == '-' ||
		aOperation[0] == '*' ||
		aOperation[0] == '/' ||
		(aOperation[0] == '!' &&
			aOperation[1] == '='))
	{
		printf("Syntax error (op starting with '%c') \"%s\", %s line %d\n", aOperation[0], aOperation, gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
		exit(-1);
	}

	int i = 0;
	int operations = 0;
	while (aOperation[i])
	{
		if (aOperation[i] == ':' ||
			(aOperation[i] == '-' && aOperation[i + 1] != '=') ||
			(aOperation[i] == '+' && aOperation[i + 1] != '=') ||
			(aOperation[i] == '*' && aOperation[i + 1] != '=') ||
			(aOperation[i] == '/' && aOperation[i + 1] != '=') ||
			(aOperation[i] == '<' && aOperation[i + 1] != '=') ||
			(aOperation[i] == '>' && aOperation[i + 1] != '=') ||
			(aOperation[i] == '=' && aOperation[i + 1] != '='))
		{
			operations++;
		}
		if ((aOperation[i] == '-' && aOperation[i + 1] == '=') ||
			(aOperation[i] == '+' && aOperation[i + 1] == '=') ||
			(aOperation[i] == '*' && aOperation[i + 1] == '=') ||
			(aOperation[i] == '/' && aOperation[i + 1] == '=') ||
			(aOperation[i] == '<' && aOperation[i + 1] == '=') ||
			(aOperation[i] == '>' && aOperation[i + 1] == '=') ||
			(aOperation[i] == '=' && aOperation[i + 1] == '=') ||
			(aOperation[i] == '!' && aOperation[i + 1] == '='))
		{
			operations++;
			i++;
		}

		i++;
	}

	if (operations > 1)
	{
		printf("Syntax error (op with more than one instruction) \"%s\", %s line %d\n", aOperation, gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
		exit(-1);
	}

	if (operations == 0)
	{
		if (aOperation[0] == '!')
		{
			set_op(OP_NOT, gSymbol.getId(aOperation + 1));
		}
		else
		{
			set_op(OP_HAS, gSymbol.getId(aOperation));
		}
	}
	else
	{
		char cmd[256];
		char* sym;
		i = 0;
		while (
			aOperation[i] != ':' &&
			aOperation[i] != '<' &&
			aOperation[i] != '>' &&
			aOperation[i] != '=' &&
			aOperation[i] != '!' &&
			aOperation[i] != '*' &&
			aOperation[i] != '/' &&
			aOperation[i] != '+' &&
			aOperation[i] != '-')
		{
			cmd[i] = aOperation[i];
			i++;
		}
		cmd[i] = 0;
		if (aOperation[i] == ':')
		{
			sym = aOperation + i + 1;

			if (stricmp(cmd, "has")    == 0) set_op(OP_HAS, gSymbol.getId(sym)); else
			if (stricmp(cmd, "need")   == 0) set_op(OP_HAS, gSymbol.getId(sym)); else
			if (stricmp(cmd, "not")    == 0) set_op(OP_NOT, gSymbol.getId(sym)); else
			if (stricmp(cmd, "set")    == 0) set_op(OP_SET, gSymbol.getId(sym)); else
			if (stricmp(cmd, "clear")  == 0) set_op(OP_CLR, gSymbol.getId(sym)); else
			if (stricmp(cmd, "clr")    == 0) set_op(OP_CLR, gSymbol.getId(sym)); else
			if (stricmp(cmd, "toggle") == 0) set_op(OP_XOR, gSymbol.getId(sym)); else
			if (stricmp(cmd, "xor")    == 0) set_op(OP_XOR, gSymbol.getId(sym)); else
			if (stricmp(cmd, "flip")   == 0) set_op(OP_XOR, gSymbol.getId(sym)); else
			if (stricmp(cmd, "print")  == 0) set_op(OP_PRINT, gSymbol.getId(sym)); else
			if (stricmp(cmd, "random") == 0) set_op(OP_RND, atoi(sym)); else
			if (stricmp(cmd, "rand")   == 0) set_op(OP_RND, atoi(sym)); else
			if (stricmp(cmd, "rnd")    == 0) set_op(OP_RND, atoi(sym)); else
			if (stricmp(cmd, "go")     == 0) set_go_op(OP_GO, gSymbol.getId(sym)); else
			if (stricmp(cmd, "goto")   == 0) set_go_op(OP_GO, gSymbol.getId(sym)); else
			if (stricmp(cmd, "gosub")  == 0) set_go_op(OP_GOSUB, gSymbol.getId(sym)); else
			if (stricmp(cmd, "call")   == 0) set_go_op(OP_GOSUB, gSymbol.getId(sym)); else
			{
				printf("Syntax error: unknown operation \"%s\", %s line %d\n", cmd, gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
				exit(-1);
			}
		}
		else
		{
			int first = gNumber.getId(cmd);
			// numeric op <,>,<=,>=,==,!=,=,+,-,*,/
			int v = 0;
			if (aOperation[i] == '<' && aOperation[i + 1] != '=') v = OP_LT;
			if (aOperation[i] == '<' && aOperation[i + 1] == '=') v = OP_LTE;
			if (aOperation[i] == '>' && aOperation[i + 1] != '=') v = OP_GT;
			if (aOperation[i] == '>' && aOperation[i + 1] == '=') v = OP_GTE;
			if (aOperation[i] == '=' && aOperation[i + 1] != '=') v = OP_ASSIGN;
			if (aOperation[i] == '=' && aOperation[i + 1] == '=') v = OP_EQ;
			if (aOperation[i] == '!' && aOperation[i + 1] == '=') v = OP_IEQ;
			if (aOperation[i] == '+' && aOperation[i + 1] != '=') v = OP_ADD; // allow a+b
			if (aOperation[i] == '-' && aOperation[i + 1] != '=') v = OP_SUB;
			if (aOperation[i] == '+' && aOperation[i + 1] == '=') v = OP_ADD; // allow a+=b
			if (aOperation[i] == '-' && aOperation[i + 1] == '=') v = OP_SUB;
			if (aOperation[i] == '*' && aOperation[i + 1] != '=') v = OP_MUL; // allow a*b
			if (aOperation[i] == '/' && aOperation[i + 1] != '=') v = OP_DIV;
			if (aOperation[i] == '*' && aOperation[i + 1] == '=') v = OP_MUL; // allow a*=b
			if (aOperation[i] == '/' && aOperation[i + 1] == '=') v = OP_DIV;

			if (v == 0)
			{
				printf("Parse error near \"%s\" (\"%s\"), %s line %d\n", aOperation + i, aOperation, gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
				exit(-1);
			}

			sym = aOperation + i + 1;
			if (aOperation[i + 1] == '=') sym++;
			int second = 0;
			if (is_numeric(sym))
			{
				v++;
				second = atoi(sym);
			}
			else
			{
				second = gNumber.getId(sym);
			}
			set_number_op(v, first, second);
		}
	}
}
