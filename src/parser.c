#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <stdarg.h>
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

#if defined(__GNUC__)
#define Unreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
#define Unreachable() __assume(false);
#endif

#define Unimplemented() fprintf(stderr, "Function %s() not implemented : %d.", __func__, __LINE__);
#define Assert(str)                                                                                                    \
    {                                                                                                                  \
        if (!(str))                                                                                                    \
        {                                                                                                              \
            fprintf(stderr, "Function : %s() Line : %d Assertion Failed : %s.\n", __func__, __LINE__, #str);           \
            abort();                                                                                                   \
        }                                                                                                              \
    }

void print_dummy(int a, ...)
{
    fprintf(stderr, "Can't format");
}

#define print_unknown(...) print_dummy(0, __VA_ARGS__)

void unknown_print(const char *str, ...)
{
    fprintf(stderr,
            "%s : "
            "Can't format",
            str);
}
// Generic print function
#define print_generic(x) _Generic((x), int : print_int, float : print_float, default : unknown_print)(#x, x)

#define GEN_PRINT(TYPE, SPECIFIER)                                                                                     \
    void print_##TYPE(const char *str, TYPE x)                                                                         \
    {                                                                                                                  \
        return fprintf(stdout, "%s -> " SPECIFIER "\n", str, x);                                                       \
    }

#define NamedAssert(C, X)                                                                                              \
    {                                                                                                                  \
        if (!(C))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "Function : %s() Line : %d Assertion : %s.\nPresent Value : ", __func__, __LINE__, #C);    \
            print_generic(X);                                                                                          \
            abort();                                                                                                   \
        }                                                                                                              \
    }

GEN_PRINT(int, "%d");
GEN_PRINT(unsigned, "%u");
GEN_PRINT(float, "%f");

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
    OP_ADD,
    OP_SUB,
    OP_DIV,
    OP_MUL,
    OP_EXP,
    OP_EQUAL,
    OP_NONE
} Op;

typedef enum
{
    NODE,
    LEAF
} NodeType;

typedef enum TermType
{
    TERM_FUNC, // for function definitions and calling
    TERM_ID,   // for simple variables with values
    TERM_VALUE
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
    } value;
} Terminal;

typedef struct ExprTree
{
    NodeType node_type;

    union {
        Op       operation;
        Terminal term;
    } data;

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

// EResult
uint32_t EvalExprTree(ExprTree *expr)
{
    // Every numerals are uint32_t based so,
    if (expr->node_type == LEAF)
    {
        if (expr->data.term.type == TERM_VALUE)
        {
            return expr->data.term.value.value;
        }
        else
            Assert(!"Only numerals allowed for now");
    }

    // else its interior node of the tree

    switch (expr->data.operation)
    {
    case OP_ADD:
        return EvalExprTree(expr->left) + EvalExprTree(expr->right);
    case OP_SUB:
        return EvalExprTree(expr->left) - EvalExprTree(expr->right);
    case OP_MUL:
        return EvalExprTree(expr->left) * EvalExprTree(expr->right);
    case OP_DIV:
        return EvalExprTree(expr->left) / EvalExprTree(expr->right);
    default:
        Assert(!"Unsupported Operation ....");
    }
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
Token TokenizeNext(Tokenizer *tokenizer);

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

        switch (tokenizer->buffer.data[tokenizer->buffer.pos++])
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
                tokenizer->buffer.pos--;

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
    Tokenizer *tokenizer;
    Token      current_token;
    // its not going to be generic, so working for this specific case only, we have
} Parser;

Token ParseVar(Parser *parser);
void  ParserStart(Parser *parser)
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

ExprTree *ParseS_(Parser *parser, ExprTree *inherited_tree);
ExprTree *ParseS(Parser *parser);

ExprTree *ParseF(Parser *parser)
{
    TokenType type = parser->current_token.type;
    if (type == TOKEN_ID || type == TOKEN_NUM)
    {
        ExprTree *expr_tree = malloc(sizeof(*expr_tree));
        Assert(expr_tree != NULL);
        expr_tree->left           = NULL;
        expr_tree->right          = NULL;

        expr_tree->node_type      = LEAF;
        expr_tree->data.term.type = type == TOKEN_NUM ? TERM_VALUE : TERM_ID;

        if (type == TOKEN_NUM)
            expr_tree->data.term.value.value = parser->current_token.token_num.value;
        else
            // TODO :: Update when string type is updated
            strcpy(expr_tree->data.term.value.id,
                   parser->current_token.token_id.name); // Len value ignored assuming c style strings

        parser->current_token = TokenizeNext(parser->tokenizer);

        return expr_tree;
    }
    if (type == TOKEN_OPAREN)
    {
        parser->current_token = TokenizeNext(parser->tokenizer);
        ExprTree *tree        = ParseS(parser);
        Assert(parser->current_token.type == TOKEN_CPAREN);
        parser->current_token = TokenizeNext(parser->tokenizer);
        return tree;
    }
    Assert(!"Invalid input detected...");
    Unreachable();
}

ExprTree *ParseT_(Parser *parser, ExprTree *inherited_tree) // Takes the inherited attributes
{
    TokenType type = parser->current_token.type;
    if (type == TOKEN_MUL || type == TOKEN_DIV)
    {
        ExprTree *expr_tree = malloc(sizeof(*expr_tree));
        Assert(expr_tree != NULL);
        expr_tree->node_type      = NODE;
        expr_tree->data.operation = type == TOKEN_MUL ? OP_MUL : OP_DIV;
        expr_tree->left           = inherited_tree;

        // Order of parsing matters here
        parser->current_token = TokenizeNext(parser->tokenizer);
        expr_tree->right      = ParseF(parser);

        return ParseT_(parser, expr_tree);
    }
    return inherited_tree;
}

ExprTree *ParseT(Parser *parser)
{

    if (parser->current_token.type == TOKEN_NUM || parser->current_token.type == TOKEN_ID ||
        parser->current_token.type == TOKEN_OPAREN)
    {
        ExprTree *termtree = ParseF(parser);
        return ParseT_(parser, termtree);
    }
    else
    {
        fprintf(stderr, "Error, unexpected token found.\n");
        Unimplemented();
        return NULL;
    }
}

ExprTree *ParseS_(Parser *parser, ExprTree *inherited_tree) // token_plus and token_minus takes the inherited attributes
{
    TokenType type = parser->current_token.type;
    if (type == TOKEN_PLUS || type == TOKEN_MINUS)
    {
        // Allocate a new tree with node
        ExprTree *expr_tree = malloc(sizeof(*expr_tree));
        Assert(expr_tree != NULL);
        expr_tree->node_type      = NODE;
        expr_tree->data.operation = type == TOKEN_PLUS ? OP_ADD : OP_SUB;
        expr_tree->left           = inherited_tree;

        // Order of parsing matters here
        parser->current_token = TokenizeNext(parser->tokenizer);
        expr_tree->right      = ParseT(parser);

        return ParseS_(parser, expr_tree);
    }
    return inherited_tree;
}

ExprTree *ParseS(Parser *parser)
{
    if (parser->current_token.type == TOKEN_NUM || parser->current_token.type == TOKEN_ID ||
        parser->current_token.type == TOKEN_OPAREN)
    {
        ExprTree *termtree = ParseT(parser);
        return ParseS_(parser, termtree);
    }
    else
    {
        fprintf(stderr, "Error, unexpected token found.\n");
        Unimplemented();
        return NULL;
    }
}

ExprTree *CreateExprTree(Parser *parser)
{
    parser->current_token = TokenizeNext(parser->tokenizer);
    return ParseS(parser);
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
        strcpy(symbol.id, token.token_id.name); // ignore length for now
        TokenizeNext(tokenizer);                // skip the lookahead
        // ParserVarBody(parser, &symbol);
    }
}

int main(int argc, char **argv)
{
    const char *string = "1 + 2 * (3 * 4 + 44 / 4 * 2) / 4 ";
    // const char *string    = "44 / 4 * 2";
    Tokenizer *tokenizer = CreateTokenizer(string, strlen(string));
    // Token       tok       = TokenizeNext(tokenizer);
    // PrintToken(&tok);
    Parser parser;
    parser.tokenizer = tokenizer;
    ExprTree *expr   = CreateExprTree(&parser);
    fprintf(stderr, "Output : %u.", EvalExprTree(expr));

    return 0;
}