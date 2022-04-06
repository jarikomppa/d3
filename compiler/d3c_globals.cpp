/*
DialogTree (d3) engine compiler
Copyright (c) 2020 Jari Komppa
http://iki.fi/sol
Released under Unlicense

See d3c.h for details
*/

#include "d3c.h"

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
bool gDumpDot = false;
bool gImplicitFlags = true;
char* gPrefix = 0;
char* gGlobalPage = 0;
int gGlobalPageId = 0;

int gCommandPtrOpOfs = 0;

char gScratch[64 * 1024];
char gText[64 * 1024];
int gTextIdx;
