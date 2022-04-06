/*
DialogTree (d3) engine compiler
Copyright (c) 2020 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"

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

void output_json(FILE* f)
{
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
	if (gJsonOutput)
	{
		output_json(f);
	}
	else
	{
		output_binary(f);
	}
	fclose(f);
}
