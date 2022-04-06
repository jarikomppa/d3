/*
DialogTree (d3) engine compiler
Copyright (c) 2020 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"


void open_file(char* aFilename)
{
	gFile++;
	if (gFile == MAX_FILESTACK)
	{
		printf("Error: too many nested includes\n");
		exit(-1);
	}
	if (gFileStack[gFile].mFile)
	{
		printf("Internal Error: trying to open a file which is open\n");
		exit(-1);
	}
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	fopen_s(&(gFileStack[gFile].mFile), aFilename, "rb");
#else
	gFileStack[gFile].mFile = fopen(aFilename, "rb");
#endif
	if (!gFileStack[gFile].mFile)
	{
		printf("Error: can't open file '%s'\n", aFilename);
		exit(-1);
	}
	gFileStack[gFile].mFilename = mystrdup(aFilename);
	gFileStack[gFile].mLine = 0;
}

void close_file()
{
	if (gFile == -1)
	{
		printf("Internal Error: trying to close file while file stack is empty\n");
		exit(-1);
	}
	if (gFileStack[gFile].mFile == 0)
	{
		printf("Internal Error: trying to close file that's not open\n");
		exit(-1);
	}
	fclose(gFileStack[gFile].mFile);
	delete[] gFileStack[gFile].mFilename;
	gFileStack[gFile].mFile = 0;
	gFileStack[gFile].mFilename = 0;
	gFileStack[gFile].mLine = 0;
	gFile--;
}


void trim_trailing_whitespace(char* aText)
{
	if (!aText || !*aText) return;

	int i = 0;
	while (aText[i]) i++;
	if (!is_whitespace(aText[i - 1])) return;
	while (is_whitespace(aText[i - 1])) i--;
	aText[i] = 0;
}

void token(int aTokenIndex, char* aSource, char* aDestination)
{
	while (aTokenIndex && *aSource)
	{
		while (*aSource && !is_whitespace(*aSource)) aSource++;
		while (*aSource && is_whitespace(*aSource)) aSource++;
		aTokenIndex--;
	}
	while (*aSource && !is_whitespace(*aSource))
	{
		*aDestination = *aSource;
		aDestination++;
		aSource++;
	}
	*aDestination = 0;
}

void read_raw_line(char* aBuffer, FILE* aFile)
{
	int i = 0;
	int c;
	do
	{
		c = fgetc(aFile);
		// Ignore carrier returns. Presume text files to be in either
		// dos or unix format. Unlikely to see mac format text files
		// now that mac is a unix.
		if (c == '\r')
			c = fgetc(aFile);
		// Turn tabs to spaces
		if (c == '\t')
			c = ' ';
		// Turn end of file to newline
		if (feof(aFile))
			c = '\n';
		aBuffer[i] = c;
		if (!feof(aFile) && c != '\n')
			i++;
	} while (!feof(aFile) && c > 31);

	// trim trailing spaces:
	while (i >= 0 && is_whitespace(aBuffer[i])) i--;
	i++;

	// terminate
	aBuffer[i] = 0;
}

// Skips commented lines
void read_line(char* aBuffer)
{
	char t[256];
	do
	{
		read_raw_line(aBuffer, gFileStack[gFile].mFile);
		trim_trailing_whitespace(aBuffer);
		gFileStack[gFile].mLine++;
		if (aBuffer[0] == '$' && aBuffer[1] == 'I')
		{
			// Include statement.
			token(1, aBuffer, t);
			open_file(t);
			aBuffer[0] = 0;
		}
		if (aBuffer[0] == 0 && feof(gFileStack[gFile].mFile) && gFile > 0)
		{
			close_file();
			aBuffer[0] = 0;
		}
	} while (!feof(gFileStack[0].mFile) && (aBuffer[0] == '#'));
}

void start_card()
{
	Card* c = new Card();
	if (gCardRoot == 0)
	{
		gCardRoot = c;
	}
	else
	{
		gCurrentCard->mNext = c;
	}
	gCurrentCard = c;

	gCurrentSection = 0;
	gCurrentParagraph = 0;
	gCurrentOp = 0;
}

void start_paragraph()
{
	if (gCurrentParagraph != 0 && gCurrentParagraph->mOp == 0 && gCurrentParagraph->mText == 0)
	{
		// No need to create a new paragraph because current is unused.
		return;
	}

	Paragraph* p = new Paragraph();
	if (gCurrentParagraph == 0)
	{
		gCurrentSection->mParagraph = p;
	}
	else
	{
		gCurrentParagraph->mNext = p;
	}
	gCurrentParagraph = p;

	gCurrentOp = 0;
}

void start_section(char aSectionType, int id)
{
	Section* s = new Section();
	s->mQuestion = (aSectionType == 'Q');
	s->mSymbol = id;
	if (gCurrentSection == 0)
	{
		gCurrentCard->mSection = s;
	}
	else
	{
		gCurrentSection->mNext = s;
	}
	gCurrentSection = s;

	gCurrentParagraph = 0;
	gCurrentOp = 0;

	start_paragraph();
}

void paragraph_break()
{
	// add a newline
	if (gTextIdx > 0 && gText[gTextIdx - 1] == ' ')
		gTextIdx--;
	gText[gTextIdx] = '\n';
	gTextIdx++;
}

void parse_statement()
{
	// parse statement
	gCommandPtrOpOfs = 0;
	int i;
	char t[256];
	switch (gScratch[1])
	{
	case 'Q':
		token(1, gScratch, t);
		i = gSymbol.getId(t);
		gCommandPtrOpOfs = 2; // $Q + cardno
		start_card();
		start_section('Q', i);
		if (gVerbose) printf("Card: \"%s\" (%d)\n", t, i);
		if (gImplicitFlags)
			set_op(OP_SET, i);
		if (gGlobalPage && i != gGlobalPageId)
			set_go_op(OP_GOSUB, gGlobalPageId);
		break;
	case 'A':
		token(1, gScratch, t);
		i = gSymbol.getId(t);
		gCommandPtrOpOfs = 2; // $A + cardno
		start_section('A', i);
		if (gVerbose) printf("Choice: %s (%d)\n", t, i);
		break;
	case 'C':
		if (gVerbose) printf("Config\n");
		gCommandPtrOpOfs = 10000;
		break;
	case 'G':
		if (gVerbose) printf("Global page config\n");
		gCommandPtrOpOfs = 10000;
		break;
	case 'O':
		if (gVerbose) printf("Predicated continuing paragraph\n");
		if (gCurrentParagraph->mText)
		{
			// Remove potential newline(s) from the end of previous paragraph
			int p = 0;
			while (gCurrentParagraph->mText[p]) p++;
			while (gCurrentParagraph->mText[p - 1] == '\n') p--;
			gCurrentParagraph->mText[p] = 0;
		}
		// fallthrough
	case 'P':
		gCommandPtrOpOfs = 1; // $O (or $P)
		start_paragraph();
		if (gVerbose && gScratch[1] == 'P') printf("Predicated paragraph\n");
		break;
	default:
		printf("Syntax error: unknown statement \"%s\", %s line %d\n", gScratch, gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
		exit(-1);
	}

	i = gCommandPtrOpOfs;
	do
	{
		token(i, gScratch, t);
		if (t[0]) parse_op(t);
		i++;
	} while (t[0]);

	if (gScratch[1] == 'Q')
	{
		// Create a new paragraph at start of a 'Q' block to avoid
		// the 'Q' blocks' text getting (potentially) predicated.
		// This means the 'Q' blocks never have text by themselves.
		start_paragraph();
	}
}

void store_text(char* aText)
{
	// Skip garbage text before the first $Q.
	// A bit of a hack, but less effort this way.
	if (!gCurrentParagraph)
		return;

	if (gVerbose)
	{
		printf("    Text:\n%s\n\"\"\"\n", aText);
	}
	gCurrentParagraph->mText = mystrdup(aText);
}

void flush_text()
{
	// remove singular trailing space
	if (gTextIdx > 0 && gText[gTextIdx - 1] == ' ')
		gTextIdx--;
	if (gTextIdx == 0)
		return;
	gText[gTextIdx] = 0;
	store_text(gText);

	gTextIdx = 0;
	gText[0] = 0;
}

void capture_text()
{
	// capture string literal
	char* s = gScratch;
	int column = 0;

	if (*s == 0 || *s == '\n')
	{
		paragraph_break();
		return;
	}

	int was_whitespace = 1;

	while (*s)
	{
		int ws = is_whitespace(*s);
		if (ws)
		{
			if (!was_whitespace)
			{
				ws = 0;
				*s = ' ';
			}
			was_whitespace = 1;
		}
		else
		{
			was_whitespace = 0;
		}

		if (!ws)
		{
			if (s[0] == '<' && s[1] == '<')
			{
				s += 2;
				column += 2;
				char temp[256];
				// number output
				int i = 0;
				while (!is_whitespace(s[0]) && s[0] != 0 && !(s[0] == '>' && s[1] == '>'))
				{
					temp[i] = *s;
					column++;
					s++;
					i++;
				}
				temp[i] = 0;
				if (!(s[0] == '>' && s[1] == '>'))
				{
					printf("Error parsing numeric output on %s line %d, near column %d\n", gFileStack[gFile].mFilename, gFileStack[gFile].mLine, column);
					printf("\"%s\"\n", gScratch);
					for (i = 0; i < column; i++)
						printf(" ");
					printf("^\n");

					exit(-1);
				}
				s++;
				column++;
				// If there's a space before this number, duplicate it,
				// because we're trimming final trailing space from strings.
				if (gTextIdx > 0 && gText[gTextIdx - 1] == ' ')
				{
					gText[gTextIdx] = ' ';
					gTextIdx++;
				}
				// no paragraph_break();
				flush_text();
				Paragraph* pp = gCurrentParagraph;
				start_paragraph();
				// Copy predicates from previous paragraph, because user doesn't
				// need to know we've split the paragraph.
				if (gVerbose) printf("Predicated section\n");
				Op* ow = pp->mOp;
				while (ow)
				{
					if (is_predicate(ow->mOpcode))
						store_cmd(ow->mOpcode, ow->mOperand1, ow->mOperand2);
					ow = ow->mNext;
				}
				set_number_op(OP_PRINT, gNumber.getId(temp));
				was_whitespace = 0; // don't remove any whitespace after >>
			}
			else
			{
				gText[gTextIdx] = *s;
				gTextIdx++;
			}
		}

		s++;
		column++;
	}
	// Add a space, so that string continuation doesn't cause newlines to become nothing.
	gText[gTextIdx] = ' ';
	gTextIdx++;

	gText[gTextIdx] = 0;
}

void scan_second_pass(char* aFilename)
{
	if (gGlobalPage)
	{
		if (!gSymbol.isSymbol(gGlobalPage))
		{
			printf("Error: The set global page '%s' does not refer to any existing page.\n", gGlobalPage);
			exit(1);
		}
		gGlobalPageId = gSymbol.getId(gGlobalPage);
	}

	open_file(aFilename);

	if (gVerbose)
	{
		printf("\n");
		printf("Parsing %s:\n", aFilename);
	}

	gTextIdx = 0;
	gText[0] = 0;

	while (!feof(gFileStack[0].mFile))
	{
		read_line(gScratch);
		if (gScratch[0] == '$')
		{
			if (gScratch[1] == 'C')
			{
				// Skip config statement
			}
			else
				if (gScratch[1] == 'G')
				{
					// Skip global page statement
				}
				else
				{
					// process last text - do a paragraph break
					paragraph_break();
					flush_text();
				}
			// opcode
			parse_statement();
		}
		else
		{
			// text
			capture_text();
		}
	}
	// process final text	
	flush_text();
	close_file();

	if (gVerbose)
	{
		printf("\n");
	}
}


void scan_first_pass(char* aFilename)
{
	open_file(aFilename);

	int i = 0;
	while (!feof(gFileStack[0].mFile))
	{
		read_line(gScratch);
		if (gScratch[0] == '$')
		{
			char t[256];
			token(1, gScratch, t);
			if (gScratch[1] == 'Q')
			{
				i = gSymbol.getId(t);
				if (gSymbol.mHits[i] > 1)
				{
					printf("syntax error: card id \"%s\" used more than once, %s line %d\n", t, gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
					exit(-1);
				}
				gSymbol.mHits[i]--; // clear the hit, as it'll be scanned again
			}
			if (gScratch[1] == 'G')
			{
				if (t[0] == 0)
				{
					printf("Warning: global page statement with no parameters\n");
				}
				else
				{
					if (gGlobalPage)
					{
						printf("Warning: global page already set, replacing '%s' with '%s'\n", gGlobalPage, t);
						delete[] gGlobalPage;
					}
					gGlobalPage = mystrdup(t);
					token(2, gScratch, t);
					if (t[0])
					{
						printf("Warning: unexpected token(s) found after global page setup:%s\n", gScratch);
					}
				}
			}
			if (gScratch[1] == 'C')
			{
				if (t[0] == 0)
				{
					printf("Warning: config statement with no parameters\n");
				}
				else
				{
					if (gPrefix)
					{
						printf("Warning: prefix already set, replacing '%s' with '%s'\n", gPrefix, t);
						delete[] gPrefix;
					}

					if (!is_oksymbol(t))
					{
						printf("Warning: prefix '%s' contains bad characters, please use a-z,A-Z,0-9,_\n", t);
					}
					gPrefix = mystrdup(t);

					int i = 2;
					token(i, gScratch, t);
					while (t[0])
					{
						if (stricmp(t, "implicitflags") == 0)
						{
							gImplicitFlags = true;
						}
						else
							if (stricmp(t, "noimplicitflags") == 0)
							{
								gImplicitFlags = false;
							}
							else
							{
								printf("Warning: undefined option '%s'\n", t);
							}
						i++;
						token(i, gScratch, t);
					}
				}
			}
		}
		else
		{
			// string literal
		}
	}
	gMaxCardSymbol = gSymbol.mCount;
	close_file();
}


void patch_tree_pass()
{
	Card* cw = gCardRoot;
	while (cw)
	{
		Section* sw = cw->mSection;
		while (sw)
		{
			if (sw->mQuestion)
			{
				Paragraph* pw = sw->mParagraph;
				while (pw)
				{
					Op* ow = pw->mOp;
					int has_gosub_with_text = 0;
					while (ow)
					{
						if (ow->mOpcode == OP_GO && pw->mText)
						{
							if (!(pw->mText[0] == '\n' || pw->mText[0] == '0'))
							{
								printf("Warning: $O block with goto opcode and text. Text would never be shown, so it's removed:\n\"%s\"\n", pw->mText);
							}
							delete[] pw->mText;
							pw->mText = 0;
						}
						ow = ow->mNext;
					}
					// re-scan, because someone might have both gosub and goto in same code block.
					ow = pw->mOp;
					while (ow)
					{
						if (ow->mOpcode == OP_GOSUB && pw->mText)
						{
							has_gosub_with_text = 1;
						}
						ow = ow->mNext;
					}
					// If we have gosub block with text, create a new block for the text.
					if (has_gosub_with_text)
					{
						Paragraph* np = new Paragraph;
						// Insert new paragraph
						np->mNext = pw->mNext;
						pw->mNext = np;
						// Move text to new paragraph
						np->mText = pw->mText;
						pw->mText = 0;
						// Copy predicate ops to the new block
						ow = pw->mOp;
						while (ow)
						{
							if (is_predicate(ow->mOpcode))
							{
								Op* op = new Op();
								op->mOpcode = ow->mOpcode;
								op->mOperand1 = ow->mOperand1;
								op->mOperand2 = ow->mOperand2;
								// note: stores copy in reverse order. 
								// Shouldn't(tm) cause problems, as failing of any pred will fail
								op->mNext = np->mOp;
								np->mOp = op;
							}
							ow = ow->mNext;
						}
					}
					pw = pw->mNext;
				}
			}
			else
			{
				// 'A' section
				Paragraph* pw = sw->mParagraph;
				while (pw)
				{
					// Trim whitespace from answers
					trim_trailing_whitespace(pw->mText);
					pw = pw->mNext;
				}
			}
			sw = sw->mNext;
		}
		cw = cw->mNext;
	}
}
