#define _CRT_SECURE_NO_WARNINGS
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define stricmp _stricmp

// 16k symbols should be enough for everybody
#define MAX_SYMBOLS (16*1024) 
// max number of numeric variables
#define MAX_NUMBERS 255 

enum opcodeval
{
	OP_NOP,

	OP_HAS,
	OP_NOT,

	OP_SET,
	OP_CLR,
	OP_XOR,

	OP_RND,

	OP_GO,
	OP_GOSUB,

	// Print value of variable
	OP_PRINT,

	// variable-constant pairs. The code below depends on the pairing.
	OP_GT,  // a>b
	OP_GTC, // a>n
	OP_LT,  // a<b
	OP_LTC, // a<n
	OP_GTE, // a>=b
	OP_GTEC,// a>=n
	OP_LTE, // a<=b
	OP_LTEC,// a<=n
	OP_EQ,  // a==b
	OP_EQC, // a==n
	OP_IEQ, // a!=b
	OP_IEQC,// a!=n

	OP_ASSIGN,  // a=b
	OP_ASSIGNC, // a=n
	OP_ADD,     // a+b
	OP_ADDC,    // a+n
	OP_SUB,     // a-b
	OP_SUBC,    // a-n
	OP_MUL,     // a*b
	OP_MULC,    // a*n
	OP_DIV,     // a/b
	OP_DIVC     // a/n
};

bool is_predicate(int aOp)
{
	switch(aOp)
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

// delete[]able string duplicate
char *mystrdup(const char* src)
{
	char *t;
	int l = strlen(src);
	t = new char[l + 1];
	memcpy(t, src, l);
	t[l] = 0;
	return t;
}

class Op
{
public:
	int mOpcode;
	int mOperand1;
	int mOperand2;
	Op* mNext;
	Op() : mOpcode(OP_NOP), mOperand1(0), mOperand2(0), mNext(0) {}
};

class Paragraph
{
public:	
	Op* mOp;
	char* mText;
	Paragraph* mNext;
	Paragraph() : mOp(0), mText(0), mNext(0) {}
};

class Section
{
public:
	int mSymbol;
	bool mQuestion; // Q or A
	Paragraph* mParagraph;
	Section* mNext;
	Section() : mSymbol(-1), mQuestion(true), mParagraph(0), mNext(0) {}
};

class Card
{
public:
	Section* mSection;
	Card* mNext;
	Card() : mSection(0), mNext(0) {};
};

class Symbol
{
public:
	char* mName[MAX_SYMBOLS * 2];
	int mHits[MAX_SYMBOLS * 2];
	int mHash[MAX_SYMBOLS * 2];
	int mCount;
	int mFirstIndex;

	Symbol();
	int calcHash(char* aString);
	int getId(char* aString);
};

struct FileStack
{
	char *mFilename;
	FILE *mFile;
	int mLine;
	FileStack()
	{
		mFilename = 0;
		mFile = 0;
		mLine = 0;
	}
};

// 8 deep includes should be more than enough
#define MAX_FILESTACK 8

Symbol gSymbol;
Symbol gNumber;
Card* gCardRoot = 0;
Card* gCurrentCard = 0;
Section* gCurrentSection = 0;
Paragraph* gCurrentParagraph = 0;
Op* gCurrentOp = 0;
int gFile = -1;
FileStack gFileStack[MAX_FILESTACK];

bool gJsonOutput = false;

int gCards = 0;
int gCardNo = 0;

int gMaxCardSymbol = 0;

bool gVerbose = false;
bool gQuiet = false;
bool gDumpTree = false;
bool gImplicitFlags = true;
char *gPrefix = 0;

int gCommandPtrOpOfs = 0;

char gScratch[64 * 1024];
char gText[64 * 1024];
int gTextIdx;

int gPreviousSection = 0;
int gPreviousTexts = 0;

void open_file(char *aFilename)
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
	gFileStack[gFile].mFile = fopen(aFilename, "rb");
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
	} 
	while (!feof(gFileStack[0].mFile) && (aBuffer[0] == '#' || aBuffer[0] == 0));
}


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
		sprintf(tempSym, "%s", aString);
	}
	else
	{
		sprintf(tempSym, "%s.%s", gPrefix, aString);
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


void disasm_op(int aOperation, int aParameter1, int aParameter2)
{
	switch (aOperation)
	{
	case OP_NOP:     printf("NOP"); break;
	case OP_HAS:     printf("HAS(%s)",        gSymbol.mName[aParameter1]); break;
	case OP_NOT:     printf("NOT(%s)",        gSymbol.mName[aParameter1]); break;
	case OP_SET:     printf("SET(%s)",        gSymbol.mName[aParameter1]); break;
	case OP_CLR:     printf("CLR(%s)",        gSymbol.mName[aParameter1]); break;
	case OP_XOR:     printf("XOR(%s)",        gSymbol.mName[aParameter1]); break;
	case OP_RND:     printf("RND(%d)",        aParameter1); break;
	case OP_GO:      printf("GO(%s)",         gSymbol.mName[aParameter1]); break;
	case OP_GOSUB:   printf("GOSUB(%s)",      gSymbol.mName[aParameter1]); break;
	case OP_PRINT:   printf("PRINT(%s)",      gNumber.mName[aParameter1]); break;
	case OP_GT:      printf("GT(%s,%s)",      gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_GTC:     printf("GTC(%s,%d)",     gNumber.mName[aParameter1], aParameter2); break;
	case OP_LT:      printf("LT(%s,%s)",      gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_LTC:     printf("LTC(%s,%d)",     gNumber.mName[aParameter1], aParameter2); break;
	case OP_GTE:     printf("GTE(%s,%s)",     gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_GTEC:    printf("GTEC(%s,%d)",    gNumber.mName[aParameter1], aParameter2); break;
	case OP_LTE:     printf("LTE(%s,%s)",     gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_LTEC:    printf("LTEC(%s,%d)",    gNumber.mName[aParameter1], aParameter2); break;
	case OP_EQ:      printf("EQ(%s,%s)",      gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_EQC:     printf("EQC(%s,%d)",     gNumber.mName[aParameter1], aParameter2); break;
	case OP_IEQ:     printf("IEQ(%s,%s)",     gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_IEQC:    printf("IEQC(%s,%d)",    gNumber.mName[aParameter1], aParameter2); break;
	case OP_ASSIGN:  printf("ASSIGN(%s,%s)",  gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_ASSIGNC: printf("ASSIGNC(%s,%d)", gNumber.mName[aParameter1], aParameter2); break;
	case OP_ADD:     printf("ADD(%s,%s)",     gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_ADDC:    printf("ADDC(%s,%d)",    gNumber.mName[aParameter1], aParameter2); break;
	case OP_SUB:     printf("SUB(%s,%s)",     gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_SUBC:    printf("SUBC(%s,%d)",    gNumber.mName[aParameter1], aParameter2); break;
	case OP_MUL:     printf("MUL(%s,%s)",     gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_MULC:    printf("MULC(%s,%d)",    gNumber.mName[aParameter1], aParameter2); break;
	case OP_DIV:     printf("DIV(%s,%s)",     gNumber.mName[aParameter1], gNumber.mName[aParameter2]); break;
	case OP_DIVC:    printf("DIVC(%s,%d)",    gNumber.mName[aParameter1], aParameter2); break;
	default:
		printf("Unknown operation %d (%d, %d)", aOperation, aParameter1, aParameter2);
	}
}

void store_cmd(int aOperation, int aParameter1, int aParameter2)
{
	if (gVerbose)
	{
		printf("    Opcode: ");
		disasm_op(aOperation, aParameter1, aParameter2);
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
	s->mQuestion = aSectionType == 'Q';
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
	gPreviousTexts = 0;

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
		gPreviousSection = 'Q';
		if (gImplicitFlags)
			set_op(OP_SET, i);
		break;
	case 'A':
		token(1, gScratch, t);
		i = gSymbol.getId(t);
		gCommandPtrOpOfs = 2; // $A + cardno
		start_section('A', i);
		if (gVerbose) printf("Choice: %s (%d)\n", t, i);
		gPreviousSection = 'A';
		break;
	case 'C':
		if (gVerbose) printf("Config\n");
		gCommandPtrOpOfs = 10000;
		break;
	case 'P':
		if (gVerbose) printf("Paragraph break\n");
		gCommandPtrOpOfs = 10000;
		token(1, gScratch, t);
		if (t[0])
		{
			printf("Syntax error - statement P may not include any operations, %s line %d\n", gFileStack[gFile].mFilename, gFileStack[gFile].mLine);
			exit(-1);
		}
		break;
	case 'O':
		gCommandPtrOpOfs = 1; // $O
		start_paragraph();
		if (gVerbose) printf("Predicated section\n");
		gPreviousSection = 'O';
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
	} 
	while (t[0]);
	if (gScratch[1] == 'Q')
	{
		// Create a new paragraph at start of a 'Q' block to avoid
		// the 'Q' blocks' text getting (potentially) predicated.
		// This means the 'Q' blocks never have text by themselves.
		start_paragraph();
		gPreviousSection = 'O';
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

	if (*s == 0)
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
			if (gScratch[1] == 'P')
			{
				// Paragraph break statement
				paragraph_break();
				paragraph_break();
			}
			else
			if (gScratch[1] == 'C') 
			{
				// Skip config statement
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
						} else
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


void report()
{
	int i;
	printf("\n");
	printf("Token Hits Symbol\n");
	for (i = 0; i < gSymbol.mCount; i++)
	{
		printf("%5d %4d \"%s\"%s\n", i, gSymbol.mHits[i], gSymbol.mName[i], gSymbol.mHits[i] < 2 ? " <- Warning" : "");
	}

	if (gNumber.mCount)
	{
		printf("\n");
		printf("Token Hits Number\n");
		for (i = 0; i < gNumber.mCount; i++)
		{
			printf("%5d %4d \"%s\"%s\n", i, gNumber.mHits[i], gNumber.mName[i], gNumber.mHits[i] < 2 ? " <- Warning" : "");
		}
	}
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
				       sw->mQuestion?"Q":"A",
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
  					       "      \"t\": \"%s\"\n",gScratch);
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

void output_binary(FILE* f)
{
	// TODO
	printf("Binary output not implemented\n");
}

void output(char* aFilename)
{
	FILE* f = fopen(aFilename, "wb");
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


void dump_tree()
{
	Card* cw = gCardRoot;
	while (cw)
	{
		printf("Card\n");
		Section* sw = cw->mSection;
		while (sw)
		{
			printf(" Section (%c:%s)\n", sw->mQuestion ? 'Q' : 'A', gSymbol.mName[sw->mSymbol]);
			Paragraph* pw = sw->mParagraph;
			while (pw)
			{
				printf("  Paragraph\n");
				Op* ow = pw->mOp;
				while (ow)
				{
					printf("   Opcode:");
					disasm_op(ow->mOpcode, ow->mOperand1, ow->mOperand2);
					printf("\n");
					ow = ow->mNext;
				}
				if (pw->mText)
					printf("   Text:\n%s\n\"\"\"\n", pw->mText);
				pw = pw->mNext;
			}
			sw = sw->mNext;
		}
		cw = cw->mNext;
	}
}

void sanity_check()
{
	int i, j;
	for (i = 0; i < gSymbol.mCount; i++)
	{
		for (j = 0; j < gNumber.mCount; j++)
		{
			if (stricmp(gSymbol.mName[i], gNumber.mName[j]) == 0)
			{
				printf("Warning: Symbol \"%s\" used both as a flag and a number. This probably isn't what you wanted.\n", gSymbol.mName[i]);
			}
		}
	}

	if (gSymbol.mCount > MAX_SYMBOLS)
	{
		printf("Too many symbols in use (%d > %d)\n", gSymbol.mCount, MAX_SYMBOLS);
		exit(-1);
	}

	if (gNumber.mCount > MAX_NUMBERS)
	{
		printf("Too many numeric variables in use (%d > %d)\n", gNumber.mCount, MAX_NUMBERS);
		exit(-1);
	}
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
							printf("Warning: $O block with goto opcode and text. Text would never be shown, so it's removed:\n\"%s\"\n", pw->mText);
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
						Paragraph *np = new Paragraph;
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
								Op *op = new Op();
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

int main(int aParc, char** aPars)
{
	printf("DialogTree compiler, by Jari Komppa http://iki.fi/sol/\n");
	if (aParc < 3)
	{
		printf(
			"%s <input> <output> [flags]\n"
			"Optional flags:\n"
			"  -v   verbose (useful for debugging)\n"
			"  -q   quiet (minimal output)\n"
			"  -m   disable implicit flags by default\n"
			"  -t   output data tree after parsing\n"
			"  -j   JSON output (default binary)\n",
			aPars[0]);
		return -1;
	}
	int infile = -1;
	int outfile = -1;
	int i;
	for (i = 1; i < aParc; i++)
	{
		if (aPars[i][0] == '-')
		{
			switch (aPars[i][1])
			{
			case 'v':
			case 'V':
				gQuiet = false;
				gVerbose = true;
				break;
			case 'q':
			case 'Q':
				gQuiet = true;
				gVerbose = false;
				break;
			case 't':
			case 'T':
				gDumpTree = true;
				break;
			case 'j':
			case 'J':
				gJsonOutput = true;
				break;
			case 'm':
			case 'M':
				gImplicitFlags = false;
				break;
			default:
				printf("Unknown parameter \"%s\"\n", aPars[i]);
				exit(-1);
			}
		}
		else
		{
			if (infile == -1)
			{
				infile = i;
			}
			else
			{
				if (outfile == -1)
				{
					outfile = i;
				}
				else
				{
					printf("Invalid parameter \"%s\" (input and output files already defined)\n", aPars[i]);
					exit(-1);
				}
			}
		}
	}

	if (outfile == -1)
	{
		printf("Invalid parameters (run without params for help)\n");
		exit(-1);
	}

	scan_first_pass(aPars[infile]);

	scan_second_pass(aPars[infile]);

	patch_tree_pass();

	if (gDumpTree)
	{
		dump_tree();
	}

	if (!gQuiet) report();

	sanity_check();

	output(aPars[outfile]);

	return 0;
}