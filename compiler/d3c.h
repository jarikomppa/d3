/*
DialogTree (d3) engine compiler
Copyright (c) 2020-2023 Jari Komppa
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
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define stricmp _stricmp

// 16k symbols should be enough for everybody
#define MAX_SYMBOLS (16*1024) 
// max number of numeric variables
#define MAX_NUMBERS 255 
// 8 deep includes should be more than enough
#define MAX_FILESTACK 8

enum d3_tags
{
	D3_HEAD = 0x30303344,
	D3_CARD = 0x44524143,
	D3_SECT = 0x54434553,
	D3_PARA = 0x41524150,
	D3_SYMS = 0x534d5953
};

enum outputformats
{
	OUTPUT_BINARY,
	OUTPUT_JSON,
	OUTPUT_TAGGED
};

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
	int isSymbol(char* aString);
};

struct FileStack
{
	char* mFilename;
	FILE* mFile;
	int mLine;
	FileStack()
	{
		mFilename = 0;
		mFile = 0;
		mLine = 0;
	}
};

extern Symbol gSymbol;
extern Symbol gNumber;
extern Card* gCardRoot;
extern Card* gCurrentCard;
extern Section* gCurrentSection;
extern Paragraph* gCurrentParagraph;
extern Op* gCurrentOp;
extern int gFile;
extern FileStack gFileStack[];

extern int gOutputFormat;

extern int gCards;
extern int gCardNo;

extern int gMaxCardSymbol;

extern bool gVerbose;
extern bool gQuiet;
extern bool gDumpTree;
extern bool gDumpDot;
extern bool gImplicitFlags;
extern char* gPrefix;
extern char* gGlobalPage;
extern int gGlobalPageId;

extern int gCommandPtrOpOfs;

extern char gScratch[];
extern char gText[];
extern int gTextIdx;

void disasm_op(FILE* f, int aOperation, int aParameter1, int aParameter2);
void set_op(int aOpCode, int aValue);
void set_number_op(int aOpCode, int aValue);
void set_number_op(int aOpCode, int aValue1, int aValue2);
void set_go_op(int aOperation, int aValue);
void parse_op(char* aOperation);
bool is_predicate(int aOp);
int is_globalsymbol(char* aString);
int is_okglobalsymbol(char* aString);
int is_oksymbol(char* aString);
int is_numeric(char* aString);
int is_whitespace(char aCharacter);
char* mystrdup(const char* src);
void store_cmd(int aOperation, int aParameter);
void store_cmd(int aOperation, int aParameter1, int aParameter2);
void dump_tree(char* aBaseFilename);
void dump_dot(char* aBaseFilename);
void output(char* aFilename);
void scan_first_pass(char* aFilename);
void scan_second_pass(char* aFilename);
void patch_tree_pass();


