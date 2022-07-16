#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A Tokenizer for the expression that could be drawn in the plotter

/*
 * We want to parse expressions like :
 * f(x) = x * x
 * g(x) = x ^ 2 + 3 * 4 + x * y
 * h(x) = x ; -1 <= x <= 1
 * expr :- var = stmt | stmt
 * stmt :- var | stmt * stmt | stmt + stmt | stmt - stmt | stmt / stmt
 */

#define MAX_ID_LEN 64

typedef enum TokenType
{
    TOKEN_COMMA,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_NUM,
    TOKEN_OPAREN,
    TOKEN_CPAREN,
    TOKEN_ID,
    TOKEN_NONE,
    TOKEN_EXP,
    TOKEN_EQUAL,
    TOKEN_INVALID,
    TOKEN_END
} TokenType;

struct SymbolTable
{
    char symbol[50];
    // what to store as value?
};

typedef struct TokenVar
{
    char     id[50];

    uint32_t param_len;
    // Need recursive data types here
    struct TokenVar *params; // not much than this, its bad, look at this
} TokenVar;

typedef struct Token
{
    TokenType type;
    union {
        struct
        {
            uint16_t len;
            char     name[MAX_ID_LEN];
        } token_id;

        TokenVar token_var;

        struct
        {
            uint32_t value;
        } token_num;
    };
} Token;

typedef enum
{
    OP_PLUS,
    OP_MINUS,
    OP_DIV,
    OP_MUL,
    OP_EXP,
    OP_NONE
} Op;

typedef enum
{
    NODE,
    LEAF
} NodeType;

typedef enum TermType
{
    FUNC, // for function definitions and calling
    ID,   // for simple variables with values
    VALUE
} TermType;

typedef struct
{
    // could be num or variable or function invocation too
    // handle all these cases
    TermType type;
    union {
        uint32_t value;
        char     id[MAX_ID_LEN];
        // func not declared as of now
    }value;
} Terminal;

typedef struct ExprTree
{
    NodeType node_type;

    union {
        Op       operator;
        Terminal term;
    };

    struct ExprTree *left;
    struct ExprTree *right;
} ExprTree;

typedef struct
{
    struct
    {
        uint8_t *data;
        uint32_t len;
        uint32_t pos;
    } buffer;
} Tokenizer;

void EvalExprTree(ExprTree *expr)
{
}

bool IsAlpha(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool IsDigit(char ch)
{
    return ch <= '9' && ch >= '0';
}

bool IsAlphaNumeric(char ch)
{
    return IsAlpha(ch) || IsDigit(ch);
}

bool IsWhiteSpace(char ch)
{
    return ch == '\t' || ch == ' ' || ch == '\n';
}

// Lets try simple parsing for now
// Try to lookahead token without actually consuming it
Token TokenizerLookahead(Tokenizer *tokenizer)
{
    uint32_t pos          = tokenizer->buffer.pos;

    Token    tok          = TokenizeNext(tokenizer);
    tokenizer->buffer.pos = pos;

    return tok;
}

Token TokenizeNext(Tokenizer *tokenizer)
{
    // try parsing as number, the simplest ones
    // might need a lookahead somewhere
    Token token;
    token.type = TOKEN_NONE;

    // consume whitespace and new lines
    while (tokenizer->buffer.pos < tokenizer->buffer.len && IsWhiteSpace(tokenizer->buffer.data[tokenizer->buffer.pos]))
    {
        tokenizer->buffer.pos = tokenizer->buffer.pos + 1;
    }

    if (tokenizer->buffer.pos < tokenizer->buffer.len)
    {
        // Parse simple numbers
        if (IsDigit(tokenizer->buffer.data[tokenizer->buffer.pos]))
        {
            uint32_t value = tokenizer->buffer.data[tokenizer->buffer.pos++] - '0';
            while (IsDigit(tokenizer->buffer.data[tokenizer->buffer.pos]))
                value = value * 10 + (tokenizer->buffer.data[tokenizer->buffer.pos++]) - '0';

            token.type            = TOKEN_NUM;
            token.token_num.value = value;
            return token;
        }

        switch (tokenizer->buffer.data[tokenizer->buffer.pos])
        {
        case '(':
            token.type = TOKEN_OPAREN;
            break;
        case ')':
            token.type = TOKEN_CPAREN;
            break;
        case ',':
            token.type = TOKEN_COMMA;
            break;
        case '*':
            token.type = TOKEN_MUL;
            break;
        case '/':
            token.type = TOKEN_DIV;
            break;
        case '+':
            token.type = TOKEN_PLUS;
            break;
        case '-':
            token.type = TOKEN_MINUS;
            break;
        case '^':
            token.type = TOKEN_EXP;
            break;
        default:
            // parse as identifier
            {
                token.type         = TOKEN_ID;
                token.token_id.len = 0;

                while ((tokenizer->buffer.pos < tokenizer->buffer.len) &&
                       IsAlphaNumeric(tokenizer->buffer.data[tokenizer->buffer.pos]))
                {
                    token.token_id.name[token.token_id.len++] = tokenizer->buffer.data[tokenizer->buffer.pos];
                    tokenizer->buffer.pos                     = tokenizer->buffer.pos + 1;
                }
                token.token_id.name[token.token_id.len] = '\0';
            }
        }
    }
    return token;
}

Tokenizer *CreateTokenizer(uint8_t *buffer, uint32_t len)
{
    Tokenizer *tokenizer = malloc(sizeof(*tokenizer));
    memset(tokenizer, 0, sizeof(*tokenizer));

    // should it be deep copied or shallow copy?
    // Shallow copy for now
    tokenizer->buffer.data = buffer;
    tokenizer->buffer.len  = len;
}

void PrintToken(Token *token)
{
    switch (token->type)
    {
    case TOKEN_NUM:
        fprintf(stderr, "Token Num : Value -> %u.", token->token_num.value);
    case TOKEN_ID:
        fprintf(stderr, "Token ID  : Name  -> %s.", token->token_id.name);
    }
}

// Now start working on parser
// Using LL(1) grammar, with left recursion elimination

typedef struct
{
    Tokenizer tokenizer;
    // its not going to be generic, so working for this specific case only, we have
} Parser;

void ParserStart(Parser *parser)
{
    // its just passing a single line not a big deal though
    // first lets work only on parsing for now, and then use syntax directed translation to guide the search

    Token next_token = TokenizerLookahead(&parser->tokenizer);
    // f(x) = x

    switch (next_token.type)
    {
    case TOKEN_NONE:
        return;
    case TOKEN_ID:
    {
        // Not enough information from here
        // Parse it as a variable now
        ParseVar(&parser->tokenizer);
    }
    break;
    }
}

typedef struct ComputationContext
{
    uint32_t nothing;
} ComputationContext;

typedef struct Symbol
{
    // It can be either variable or function
    char     id_len;
    char     id[MAX_ID_LEN];
    uint32_t value;
} Symbol;

typedef struct SymbolFn
{
    uint32_t  type; // implicit 1D, 2D or HD
    uint32_t  args_count;
    char      id[MAX_ID_LEN];
    Symbol   *args;
    ExprTree *expr_tree;
} SymbolFn;

typedef struct SymbolTable
{
    uint32_t  var_count;
    uint32_t  var_max;
    uint32_t  fn_count;
    uint32_t  fn_max;

    Symbol   *variables;
    SymbolFn *functions;
} SymbolTable;

SymbolTable symbol_table;

void        InitSymbolTable(SymbolTable *symbol_table)
{
    memset(symbol_table, 0, sizeof(*symbol_table));
    symbol_table->var_max   = 10;
    symbol_table->fn_max    = 10;

    symbol_table->variables = malloc(sizeof(*symbol_table->variables) * symbol_table->var_max);
    symbol_table->functions = malloc(sizeof(*symbol_table->functions) * symbol_table->fn_max);
}

void InsertSymbolVar(SymbolTable *table, Symbol *symbol)
{
}

void InsertSymbolFn(SymbolTable *table, SymbolFn *fn)
{
}

uint32_t EvalExpr(ExprTree *tree)
{
}

void ParseT_() // Takes the inherited attributes
{
}

void ParseT(Parser* parser)
{

}

void ParseS_() // token_plus and token_minus takes the inherited attributes 
{

}

ExprTree* ParseS(Parser *parser)
{
    // Starting phase of the parser
    Token token = TokenizeNext(parser->tokenizer);

    if (token.type == TOKEN_NUM || token.type == TOKEN_ID || token.type == TOKEN_OPAREN)
    {/*
        ExprTree *expr  = malloc(sizeof(*expr)); 
        expr->node_type = LEAF; 
        expr->term.type = VALUE; 
        expr->term.value = token.token_num.value; 
        return expr; */
        ExprTree *termtree = ParseT(); 
    }
    else
    {
        fprintf(stderr, "Error, unexpected token found.\n");
        return NULL; 
    }
}

void ParseF()
{
}

uint32_t CreateExprTree(Parser *parser)
{
}

bool ParseVarBody(Parser *parser, SymbolTable *symbol_table, Symbol *symbol)
{
    // should it be calculated right here?
    // use this symbol table to calculate the current value of the variable.
    // function can't be calculated, but variables can be.

    return false;
}

bool ParseFuncBody(Parser *parser, SymbolFn *fn)
{
    return false;
}

Token ParseVar(Parser *parser)
{
    Tokenizer *tokenizer = parser->tokenizer;
    Token      token     = TokenizeNext(tokenizer);
    Token      next      = TokenizerLookahead(tokenizer);

    if (next.type == TOKEN_OPAREN)
    {
        // start of the function parsing phase
        // parse function here recursively
    }
    else if (next.type == TOKEN_EQUAL)
    {
        // its a variable, add it to the symbol entry
        Symbol symbol;
        symbol.id = token.token_id.name; // ignore length for now
        TokenizeNext(tokenizer);         // skip the lookahead
        ParserVarBody(parser, &symbol);
    }
}

int main(int argc, char **argv)
{
    const char *string    = "func(x,y)";
    Tokenizer  *tokenizer = CreateTokenizer(string, 6);
    Token       tok       = TokenizeNext(tokenizer);
    PrintToken(&tok);
    return 0;
}