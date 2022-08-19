#pragma once

#include <stdint.h>

typedef struct SymbolFn    SymbolFn;
typedef struct SymbolTable SymbolTable;
typedef struct SymbolVar   SymbolVar;
typedef struct Parser      Parser;

typedef struct ComputationContext // probably dependency graph
{
    uint32_t     var_count;
    SymbolFn    *fn;    // Function under computation
    SymbolTable *table; // Most enclosing scope of execution
    SymbolVar   *vars[10];
} ComputationContext;

Parser *CreateParser(const char *str, uint32_t len); // string that should remain valid till the parsing continues
ComputationContext *NewComputation(SymbolFn *fn);
SymbolFn           *GetLatestParsedFn();

float               EvalFromContext(ComputationContext *context, float x, float y);

void                DestroyComputationContext(ComputationContext *context);
void                UpdateParser(Parser *parser, const char *str, uint32_t len);
void                UpdateParserData(Parser *parser, const char *str, uint32_t len);
void                ParseStart(Parser *parser);
void                InitInterpreter();
