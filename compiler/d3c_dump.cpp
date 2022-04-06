/*
DialogTree (d3) engine compiler
Copyright (c) 2020 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"

void dump_dot(char* aBaseFilename)
{
	char temp[1024];
	snprintf(temp, 1024, "%s.dot", aBaseFilename);
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	FILE* f;
	fopen_s(&f, temp, "wb");
#else
	FILE* f = fopen(temp, "wb");
#endif
	fprintf(f, "digraph dialog_tree {\n");

	Card* cw = gCardRoot;
	while (cw)
	{
		char* q = 0;
		Section* sw = cw->mSection;
		while (sw)
		{
			if (sw->mQuestion)
				q = gSymbol.mName[sw->mSymbol];

			int primaryEdge = 0;
			Paragraph* pw = sw->mParagraph;
			while (pw)
			{
				Op* ow = pw->mOp;
				while (ow)
				{
					if (ow->mOpcode == OP_GO)
					{
						fprintf(f, "\t%s -> %s [ label=\"goto\", color=red, fontcolor=red, fontname=\"Arial\", fontsize=\"10.0\" ];\n", q, gSymbol.mName[ow->mOperand1]);
					}
					if (ow->mOpcode == OP_GOSUB)
					{
						fprintf(f, "\t%s -> %s [ label=\"gosub\", color=blue, fontcolor=blue, fontname=\"Arial\", fontsize=\"10.0\" ];\n", q, gSymbol.mName[ow->mOperand1]);
					}
					ow = ow->mNext;
				}

				if (pw->mText && !sw->mQuestion && !primaryEdge)
				{
					snprintf(temp, 10, "%s", pw->mText);
					fprintf(f, "\t%s -> %s [ label=\"%s%s\", color=black, fontname=\"Arial\", fontsize=\"10.0\" ];\n", q, gSymbol.mName[sw->mSymbol], temp, strlen(pw->mText) > 10 ? "..." : "");
					primaryEdge = 1;
				}
				pw = pw->mNext;
			}
			if (!sw->mQuestion && !primaryEdge)
			{
				fprintf(f, "\t%s -> %s [ label=\"??\", color=black, fontname=\"Arial\", fontsize=\"10.0\" ];\n", q, gSymbol.mName[sw->mSymbol]);
			}

			sw = sw->mNext;
		}
		cw = cw->mNext;
	}

	fprintf(f, "}\n");
	fclose(f);
}

void dump_tree(char* aBaseFilename)
{
	char temp[1024];
	snprintf(temp, 1024, "%s.tree", aBaseFilename);
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	FILE* f;
	fopen_s(&f, temp, "wb");
#else
	FILE* f = fopen(temp, "wb");
#endif

	Card* cw = gCardRoot;
	while (cw)
	{
		fprintf(f, "Card\n");
		Section* sw = cw->mSection;
		while (sw)
		{
			fprintf(f, " Section (%c:%s)\n", sw->mQuestion ? 'Q' : 'A', gSymbol.mName[sw->mSymbol]);
			Paragraph* pw = sw->mParagraph;
			while (pw)
			{
				fprintf(f, "  Paragraph\n");
				Op* ow = pw->mOp;
				while (ow)
				{
					fprintf(f, "   Opcode:");
					disasm_op(f, ow->mOpcode, ow->mOperand1, ow->mOperand2);
					fprintf(f, "\n");
					ow = ow->mNext;
				}
				if (pw->mText)
					fprintf(f, "   Text:\n%s\n\"\"\"\n", pw->mText);
				pw = pw->mNext;
			}
			sw = sw->mNext;
		}
		cw = cw->mNext;
	}
	fclose(f);
}
