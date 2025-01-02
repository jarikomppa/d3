/*
DialogTree (d3) engine compiler
Copyright (c) 2020-2025 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"

bool has_newline(const char* s)
{
	if (!s)
		return false;
	while (*s)
	{
		if (*s == '\n')
			return true;
		s++;
	}
	return false;
}

bool end_newline(const char* s)
{
	if (!s)
		return false;
	while (*s)
	{
		s++;
	}
	if (s[-1] == '\n')
		return true;
	return false;
}

void escape_string(char* s)
{
	gScratch[0] = 0;
	if (s == 0)
	{
		return;
	}

	char* o = gScratch;
	while (*s)
	{
		if (*s == '\n')
		{
			*o = '\\';
			o++;
			*o = 'n';
		}
		else
			if (*s == '"')
			{
				*o = '\\';
				o++;
				*o = '"';
			}
			else
				if (*s == '/')
				{
					*o = '\\';
					o++;
					*o = '/';
				}
				else
				{
					*o = *s;
				}
		o++;
		s++;
	}
	*o = 0;
}

void repar_string(char* s)
{
	gScratch[0] = 0;
	if (s == 0)
	{
		return;
	}

	char* o = gScratch;
	while (*s)
	{
		if (*s == '\n' && s[1])
		{
			o += snprintf(o, 65536, "</p>\n");
			o += snprintf(o, 65536, "   <%s>", has_newline(s+1) ? "p" : "o");
		}
		else
		if (*s != '\n')
		{
			*o = *s;
			o++;
		}
		s++;
	}
	*o = 0;
}

void decode_op(Op* op)
{
	switch (op->mOpcode)
	{
		
	case OP_NOP: snprintf(gScratch, 65536, "nop"); break;

	case OP_HAS: snprintf(gScratch, 65536, "%s", gSymbol.mName[op->mOperand1]); break;
	case OP_NOT: snprintf(gScratch, 65536, "!%s", gSymbol.mName[op->mOperand1]); break;

	case OP_SET: snprintf(gScratch, 65536, "set:%s", gSymbol.mName[op->mOperand1]); break;
	case OP_CLR: snprintf(gScratch, 65536, "clr:%s", gSymbol.mName[op->mOperand1]); break;
	case OP_XOR: snprintf(gScratch, 65536, "xor:%s", gSymbol.mName[op->mOperand1]); break;

	case OP_RND: snprintf(gScratch, 65536, "rnd:%d", op->mOperand1); break;

	case OP_GO: snprintf(gScratch, 65536, "go:%s", gSymbol.mName[op->mOperand1]); break;
	case OP_GOSUB: snprintf(gScratch, 65536, "gosub:%s", gSymbol.mName[op->mOperand1]); break;

			// Print value of variable
	case OP_PRINT: snprintf(gScratch, 65536, "print:%s", gNumber.mName[op->mOperand1]); break;

			// variable-constant pairs. The code below depends on the pairing.
	case OP_GT:  snprintf(gScratch, 65536, "%s>%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a>b
	case OP_GTC: snprintf(gScratch, 65536, "%s>%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a>n
	case OP_LT:  snprintf(gScratch, 65536, "%s<%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a<b
	case OP_LTC: snprintf(gScratch, 65536, "%s<%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a<n
	case OP_GTE: snprintf(gScratch, 65536, "%s>=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a>=b
	case OP_GTEC:snprintf(gScratch, 65536, "%s>=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a>=n
	case OP_LTE: snprintf(gScratch, 65536, "%s<=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a<=b
	case OP_LTEC:snprintf(gScratch, 65536, "%s<=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a<=n
	case OP_EQ:  snprintf(gScratch, 65536, "%s==%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a==b
	case OP_EQC: snprintf(gScratch, 65536, "%s==%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a==n
	case OP_IEQ: snprintf(gScratch, 65536, "%s!=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a!=b
	case OP_IEQC:snprintf(gScratch, 65536, "%s!=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a!=n

	case OP_ASSIGN:  snprintf(gScratch, 65536, "%s=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a=b
	case OP_ASSIGNC: snprintf(gScratch, 65536, "%s=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a=n
	case OP_ADD:     snprintf(gScratch, 65536, "%s+=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a+b
	case OP_ADDC:    snprintf(gScratch, 65536, "%s+=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a+n
	case OP_SUB:     snprintf(gScratch, 65536, "%s-=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a-b
	case OP_SUBC:    snprintf(gScratch, 65536, "%s-=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a-n
	case OP_MUL:     snprintf(gScratch, 65536, "%s*=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a*b
	case OP_MULC:    snprintf(gScratch, 65536, "%s*=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a*n
	case OP_DIV:     snprintf(gScratch, 65536, "%s/=%s", gNumber.mName[op->mOperand1], gNumber.mName[op->mOperand2]); break;// a/b
	case OP_DIVC:    snprintf(gScratch, 65536, "%s/=%d", gNumber.mName[op->mOperand1], op->mOperand2); break;// a/n
	}
}

void fprintf_ops(FILE *f, Op* op)
{
	if (op)
	{
		fprintf(f, " op=\"");
		bool first = true;
		while (op)
		{
			decode_op(op);
			fprintf(f, "%s%s", first ? "" : " ", gScratch);
			first = false;
			op = op->mNext;
		}
		fprintf(f, "\"");
	}
}

void output_tagged(FILE* f)
{
	printf("Output in <tagged> format\n");
	fprintf(f, "<deck>\n");
	Card* cw = gCardRoot;
	while (cw)
	{
		Section* sw = cw->mSection;
		while (sw)
		{
			Paragraph* pw = sw->mParagraph;
			if (sw->mQuestion)
			{
				fprintf(f, " <card %s", gSymbol.mName[sw->mSymbol]);
				fprintf_ops(f, pw->mOp);
				fprintf(f, ">\n");
				pw = pw->mNext;
			}
			else
			{
				fprintf(f, "  <a %s", gSymbol.mName[sw->mSymbol]);
				fprintf_ops(f, pw->mOp);
				fprintf(f, ">\n");
			}

			while (pw)
			{
				if (sw->mQuestion)
				{
					fprintf(f, "   <%s", has_newline(pw->mText) ? "p" : "o");
					fprintf_ops(f, pw->mOp);
					fprintf(f, ">");
					repar_string(pw->mText);
					fprintf(f, "%s</%s>\n", gScratch, end_newline(pw->mText) ? "p" : "o");
				}
				else
				{
					fprintf(f, "   <%s>", has_newline(pw->mText) ? "p" : "o");
					repar_string(pw->mText);
					fprintf(f, "%s</%s>\n", gScratch, end_newline(pw->mText) ? "p" : "o");
				}
				pw = pw->mNext;
			}
			if (!sw->mQuestion)
				fprintf(f, "  </a>\n");
			sw = sw->mNext;
		}
		cw = cw->mNext;
		fprintf(f, " </card>\n");
	}
	fprintf(f, "</deck>");
}

void fprintf_ops_template(FILE* f, Op* op)
{
	if (op)
	{
		while (op)
		{
			decode_op(op);
			fprintf(f, " %s", gScratch);
			op = op->mNext;
		}
	}
}

void fprintf_doublenl(FILE* f, char *s)
{
	if (!s) return;
	while (*s)
	{
		fprintf(f, "%c", *s);
		if (*s == '\n')
			fprintf(f, "%c", *s);
		s++;
	}
}

void output_template(FILE* f)
{
	printf("Output in $template format\n");
	Card* cw = gCardRoot;
	while (cw)
	{
		Section* sw = cw->mSection;
		while (sw)
		{
			Paragraph* pw = sw->mParagraph;
			if (sw->mQuestion)
			{
				fprintf(f, "$Q %s", gSymbol.mName[sw->mSymbol]);
				fprintf_ops_template(f, pw->mOp);
				fprintf(f, "\n");
				pw = pw->mNext;
			}
			else
			{
				fprintf(f, "$A %s", gSymbol.mName[sw->mSymbol]);
				fprintf_ops_template(f, pw->mOp);
				fprintf(f, "\n");
			}

			while (pw)
			{
				if (sw->mQuestion && pw->mOp)
				{
					fprintf(f, "$P");
					fprintf_ops_template(f, pw->mOp);
					fprintf(f, "\n");
					fprintf_doublenl(f, pw->mText);
				}
				else
				{
					fprintf_doublenl(f, pw->mText);
					fprintf(f, "\n");
				}
				pw = pw->mNext;
			}
			sw = sw->mNext;
		}
		cw = cw->mNext;
	}
}

void output_json(FILE* f)
{
	printf("Output in .json format\n");
	fprintf(f, "{\n \"cards\": [\n");
	Card* cw = gCardRoot;
	while (cw)
	{
		fprintf(f, "  [\n");
		Section* sw = cw->mSection;
		while (sw)
		{
			fprintf(f, "   {\n"
				"    \"type\":\"%s\",\n"
				"    \"id\":\"%d\",\n"
				"    \"p\": [\n",
				sw->mQuestion ? "Q" : "A",
				sw->mSymbol);
			Paragraph* pw = sw->mParagraph;
			while (pw)
			{
				fprintf(f, "     {\n"
					"      \"code\": [\n");
				Op* ow = pw->mOp;
				while (ow)
				{
					fprintf(f, "       {\n"
						"        \"op\":\"%d\",\n"
						"        \"a1\":\"%d\",\n"
						"        \"a2\":\"%d\"\n",
						ow->mOpcode,
						ow->mOperand1,
						ow->mOperand2);
					ow = ow->mNext;
					fprintf(f, "       }%s\n", ow ? "," : "");
				}

				escape_string(pw->mText);
				fprintf(f, "       ],\n"
					"      \"t\": \"%s\"\n", gScratch);
				pw = pw->mNext;
				fprintf(f, "     }%s\n", pw ? "," : "");
			}
			fprintf(f, "    ]\n");
			sw = sw->mNext;
			fprintf(f, "   }%s\n", sw ? "," : "");
		}
		cw = cw->mNext;
		fprintf(f, "  ]%s\n", cw ? "," : "");
	}
	fprintf(f, " ],\n"
		" \"syms\": [\n");
	int i;
	for (i = 0; i < gSymbol.mCount; i++)
	{
		fprintf(f, "  \"%s\"%s\n",
			gSymbol.mName[i],
			(i == gSymbol.mCount - 1) ? "" : ",");
	}
	fprintf(f, " ],\n"
		" \"nsyms\": [\n");
	for (i = 0; i < gNumber.mCount; i++)
	{
		fprintf(f, "  \"%s\"%s\n",
			gNumber.mName[i],
			(i == gNumber.mCount - 1) ? "" : ",");
	}

	fprintf(f, " ]\n}\n");
}


void writedword(unsigned int d, FILE* f)
{
	fwrite(&d, 1, 4, f);
}

void patchdword(unsigned int d, int ofs, FILE* f)
{
	int cp = ftell(f);
	fseek(f, ofs, SEEK_SET);
	fwrite(&d, 1, 4, f);
	fseek(f, cp, SEEK_SET);
}

void output_binary(FILE* f)
{
	printf("Output in 0xBinary format\n");
	writedword(D3_HEAD, f); // 'D300' in reverse
	int d300ofs = ftell(f);
	writedword(0, f); // placeholder
	int count = 0;
	Card* cw = gCardRoot;
	while (cw)
	{
		writedword(D3_CARD, f); // 'CARD' in reverse
		int cardofs = ftell(f);
		writedword(0, f); // placeholder
		Section* sw = cw->mSection;
		while (sw)
		{
			writedword(D3_SECT, f); // 'SECT' in reverse
			int sectofs = ftell(f);
			writedword(0, f); // placeholder
			writedword(sw->mQuestion ? 'Q' : 'A', f);
			writedword(sw->mSymbol, f);

			Paragraph* pw = sw->mParagraph;
			while (pw)
			{
				writedword(D3_PARA, f); // 'PARA' in reverse
				int paraofs = ftell(f);
				writedword(0, f); // placeholder

				int count = 0;
				Op* ow = pw->mOp;
				while (ow)
				{
					count++;
					ow = ow->mNext;
				}
				writedword(count, f);

				ow = pw->mOp;
				while (ow)
				{
					writedword(ow->mOpcode, f);
					writedword(ow->mOperand1, f);
					writedword(ow->mOperand2, f);
					ow = ow->mNext;
				}
				if (pw->mText)
				{
					int len = strlen(pw->mText);
					writedword(len, f);
					fwrite(pw->mText, 1, len, f);
				}
				else
				{
					writedword(0, f);
				}

				pw = pw->mNext;
				int paraendofs = ftell(f) + 4;
				patchdword(paraendofs - paraofs, paraofs, f);
			}
			sw = sw->mNext;
			int sectendofs = ftell(f) + 4;
			patchdword(sectendofs - sectofs, sectofs, f);
		}
		cw = cw->mNext;
		int cardendofs = ftell(f) + 4;
		patchdword(cardendofs - cardofs, cardofs, f);
	}
	writedword(D3_SYMS, f); // 'SYMS' in reverse
	int symsofs = ftell(f);
	writedword(0, f); // placeholder
	writedword(gSymbol.mCount, f);
	writedword(gNumber.mCount, f);
	int i;
	for (i = 0; i < gSymbol.mCount; i++)
	{
		int len = strlen(gSymbol.mName[i]) + 1;
		writedword(len, f);
		fwrite(gSymbol.mName[i], 1, len, f);
	}
	for (i = 0; i < gNumber.mCount; i++)
	{
		int len = strlen(gNumber.mName[i]) + 1;
		writedword(len, f);
		fwrite(gNumber.mName[i], 1, len, f);
	}

	int symsendofs = ftell(f);
	patchdword(symsendofs - symsofs, symsofs, f);

	int d300endofs = ftell(f) + 4;
	patchdword(d300endofs - d300ofs, d300ofs, f);
	fclose(f);
}

void output(char* aFilename)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	FILE* f;
	fopen_s(&f, aFilename, "wb");
#else
	FILE* f = fopen(aFilename, "wb");
#endif
	if (!f)
	{
		printf("Can't open \"%s\" for writing.\n", aFilename);
		exit(-1);
	}
	switch (gOutputFormat)
	{
	case OUTPUT_JSON:
		output_json(f);
		break;
	case OUTPUT_BINARY:
		output_binary(f);
		break;
	case OUTPUT_TAGGED:
		output_tagged(f);
		break;
	case OUTPUT_TEMPLATE:
		output_template(f);
		break;
	default:
		printf("No output\n");
	}
	fclose(f);
}

