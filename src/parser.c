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

void print_str(const char *name, const char *str)
{
    fprintf(stderr, "%s : %s.\n", name, str);
}
// Generic print function
#define print_generic(x)                                                                                               \
    _Generic((x), int : print_int, float : print_float, char * : print_str, default : unknown_print)(#x, x)

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
GEN_PRINT(char, "%c");

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
    TOKEN_NEWLINE,
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
        {
            // Use the global symbol table to find the value of the variable in use
            // The leaf could either be plain variable or a function call. Hand so accordingly

            // Assert(!"Only numerals allowed for now");
        }
    }

    // else its interior node of the tree
    // TODO :: Replace it in a data driven way

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
        /*if (tokenizer->buffer.data[tokenizer->buffer.pos] == '\n')
        {
            token.type = TOKEN_NEWLINE;
            return token;
        }*/
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
        case '=':
            token.type = TOKEN_EQUAL;
            break;
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
    Assert(tokenizer != NULL);
    memset(tokenizer, 0, sizeof(*tokenizer));

    // should it be deep copied or shallow copy?
    // Shallow copy for now
    tokenizer->buffer.data = buffer;
    tokenizer->buffer.len  = len;
}

void TokenizerSetBuffer(Tokenizer *tokenizer, uint8_t *buffer, uint32_t len)
{
    // Reset the tokenizer for new phase
    tokenizer->buffer.data = buffer;
    tokenizer->buffer.len  = len;
    tokenizer->buffer.pos  = 0;
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

// Implementation for interactive graph plotting

typedef struct ComputationContext // probably dependency graph
{
    uint32_t nothing;
} ComputationContext;

typedef enum
{
    VAR_ID,
    VAR_VALUE
} SymbolVarType;

typedef struct SymbolVar
{
    SymbolVarType var_type;
    struct
    {
        char     id_len;
        char     id[MAX_ID_LEN];
        uint32_t value;
    } data; // named just for convenience
} SymbolVar;

typedef struct SymbolFn
{
    uint32_t  type; // implicit 1D, 2D or HD
    char      id[MAX_ID_LEN];
    uint32_t  args_count;
    SymbolVar args[10];
    ExprTree *expr_tree;
} SymbolFn;

// TODO :: Upgrade symbol table to use stack based implementation
// TODO :: Use hash map for symbol table
typedef struct SymbolTable
{
    uint32_t   var_count;
    uint32_t   var_max;
    uint32_t   fn_count;
    uint32_t   fn_max;

    SymbolVar *variables;
    SymbolFn **functions;

    bool       should_evaluate;
} SymbolTable;

typedef struct SymbolTableStack
{
    uint32_t      count;
    uint32_t      max;
    SymbolTable **symbol_tables;
} SymbolTableStack;

// This stack is going to be global
SymbolTableStack symbol_table_stack;

// Now start working on parser
// Using LL(1) grammar, with left recursion elimination
SymbolTable *CheckVarInScope(SymbolTableStack *stable_stack, const char *id);
SymbolVar   *FindSymbolTableEntryVar(SymbolTable *symbol_table, const char *id);

typedef struct
{
    Tokenizer *tokenizer;
    Token      current_token;
    // its not going to be generic, so working for this specific case only, we have
} Parser;

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
        // Check if the variable is in scope, but don't try to evaluate it here??
        // strcpy(expr_tree->data.term.value.id,
        //       parser->current_token.token_id.name); // Len value ignored assuming c style strings
        {
            // If the variable is in the enclosing scope and assigned, then evaluate it.
            // But if its in a enclosing scope that explicitly says to not evaluate, then don't
            SymbolTable *table = CheckVarInScope(&symbol_table_stack, parser->current_token.token_id.name);
            Assert(table != NULL);
            if (!table->should_evaluate)
            {
                strcpy(expr_tree->data.term.value.id, parser->current_token.token_id.name);
            }
            else
            {
                SymbolVar *var = FindSymbolTableEntryVar(table, parser->current_token.token_id.name);
                NamedAssert(var != NULL, parser->current_token.token_id.name);
                expr_tree->data.term.type        = TERM_VALUE;
                expr_tree->data.term.value.value = var->data.value;
            }
        }
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

// Parse the section pointed by parser and create the resulting expression tree which needs to be further evaluated for
// the result
ExprTree *CreateExprTree(Parser *parser)
{
    parser->current_token = TokenizeNext(parser->tokenizer);
    return ParseS(parser);
}

void DestroyExprTree(ExprTree *expr_tree)
{
    if (!expr_tree)
        return;
    DestroyExprTree(expr_tree->left);
    DestroyExprTree(expr_tree->right);
    free(expr_tree);
}

SymbolTable *CreateSymbolTable(bool should_evaluate);

void         PushToSymbolTableStack(SymbolTableStack *stable_stack, SymbolTable *stable)
{
    NamedAssert(stable_stack->count < stable_stack->max, stable_stack->count);
    stable_stack->symbol_tables[stable_stack->count++] = stable;
}

SymbolTable *PopFromSymbolTableStack(SymbolTableStack *stable_stack)
{
    Assert(stable_stack->count != 0);
    stable_stack->count--;
    return stable_stack->symbol_tables[stable_stack->count];
}

SymbolTable *TopOfSymbolTableStack(SymbolTableStack *stable_stack)
{
    Assert(stable_stack->count != 0);
    return stable_stack->symbol_tables[stable_stack->count - 1];
}

// Operations on symbol_table_stack
void InitSymbolTableStack(SymbolTableStack *stable_stack)
{
    stable_stack->count         = 0;
    stable_stack->max           = 10;

    stable_stack->symbol_tables = malloc(sizeof(*stable_stack) * stable_stack->max);
    Assert(stable_stack->symbol_tables != NULL);

    // Create a symbol table
    // A default stack for global scope
    PushToSymbolTableStack(stable_stack, CreateSymbolTable(true));
}

SymbolTable *CheckVarInScope(SymbolTableStack *stable_stack, const char *id)
{
    for (int32_t scope = stable_stack->count - 1; scope >= 0; --scope)
    {
        SymbolTable *table = stable_stack->symbol_tables[scope];
        // Find symbol table entry first in var segment and then in fn segment

        for (uint32_t var = 0; var < table->var_count; ++var)
        {
            if (!strcmp(id, table->variables[var].data.id))
            {
                // return symbol_table->variables->data.value;
                return table;
            }
        }
        for (uint32_t fn = 0; fn < table->fn_count; ++fn)
        {
            if (!strcmp(id, table->functions[fn]->id))
            {
                // return symbol_table->variables->data.value;
                return table;
            }
        }
    }
    return NULL;
}

void InitSymbolTable(SymbolTable *symbol_table)
{
    NamedAssert(symbol_table != NULL, symbol_table);
    memset(symbol_table, 0, sizeof(*symbol_table));
    symbol_table->var_max   = 10;
    symbol_table->fn_max    = 10;

    symbol_table->variables = malloc(sizeof(*symbol_table->variables) * symbol_table->var_max);
    symbol_table->functions = malloc(sizeof(*symbol_table->functions) * symbol_table->fn_max);

    Assert(symbol_table->variables != NULL);
    Assert(symbol_table->functions != NULL);

    memset(symbol_table->variables, 0, sizeof(*symbol_table->variables) * symbol_table->var_max);
    memset(symbol_table->functions, 0, sizeof(*symbol_table->functions) * symbol_table->fn_max);
}

SymbolTable *CreateSymbolTable(bool should_evaluate)
{
    SymbolTable *symbol_table = malloc(sizeof(*symbol_table));
    InitSymbolTable(symbol_table);
    symbol_table->should_evaluate = should_evaluate;
    return symbol_table;
}

bool InsertSymbolVar(SymbolTable *table, SymbolVar *symbol)
{
    NamedAssert(table->var_count < table->var_max, table->var_count);
    if (table->var_count >= table->var_max)
        return false;
    table->variables[table->var_count++] = *symbol;
    return true;
}

bool InsertSymbolFn(SymbolTable *table, SymbolFn *fn)
{
    // Its quite a complicated case
    NamedAssert(table->fn_count < table->fn_max, table->fn_count);
    if (table->var_count >= table->var_max)
        return false;

    table->functions[table->fn_count++] = fn;
    return true;
}

// (LResult, bool)
SymbolVar *FindSymbolTableEntryVar(SymbolTable *symbol_table, const char *id)
{
    // Only look into the entries of var for now
    for (uint32_t var = 0; var < symbol_table->var_count; ++var)
    {
        if (!strcmp(id, symbol_table->variables[var].data.id))
        {
            // return symbol_table->variables->data.value;
            return symbol_table->variables + var;
        }
    }

    Assert("Symbol not in scope");
    return NULL;
}

bool ParseVarBody(Parser *parser, SymbolTable *symbol_table, SymbolVar *symbol)
{
    // should it be calculated right here?
    // use this symbol table to calculate the current value of the variable.
    // function can't be calculated, but variables can be.

    return false;
}

uint32_t EvalExprTreeWithSymbolTable(SymbolTable *symbol_table, ExprTree *expr)
{
    // Every numerals are uint32_t based so,
    if (expr->node_type == LEAF)
    {
        if (expr->data.term.type == TERM_VALUE)
        {
            return expr->data.term.value.value;
        }
        else
        {
            // look for the current identifier in the symbol table
            // Try to lookahead a bit and see if its a function call or just a plain variable

            // Proceeding assuming its a plain variable for now
            if (expr->data.term.type == TERM_ID)
            {
                // It could even be a function invocation
                SymbolVar *var = FindSymbolTableEntryVar(symbol_table, expr->data.term.value.id);
                NamedAssert(var != NULL, expr->data.term.value.id);
                return var->data.value;
            }

            // Assert(!"Only numerals allowed for now");
        }
    }

    // else its interior node of the tree
    // TODO :: Replace it in a data driven way

    switch (expr->data.operation)
    {
    case OP_ADD:
        return EvalExprTreeWithSymbolTable(symbol_table, expr->left) +
               EvalExprTreeWithSymbolTable(symbol_table, expr->right);
    case OP_SUB:
        return EvalExprTreeWithSymbolTable(symbol_table, expr->left) -
               EvalExprTreeWithSymbolTable(symbol_table, expr->right);
    case OP_MUL:
        return EvalExprTreeWithSymbolTable(symbol_table, expr->left) *
               EvalExprTreeWithSymbolTable(symbol_table, expr->right);
    case OP_DIV:
        return EvalExprTreeWithSymbolTable(symbol_table, expr->left) /
               EvalExprTreeWithSymbolTable(symbol_table, expr->right);
    default:
        Assert(!"Unsupported Operation ....");
    }
}

bool ParseVar(Parser *parser);

// It might need to take current symbol table to calculate its value at the moment
// TODO :: Create an interpreted session
void ParseStart(Parser *parser)
{
    // its just passing a single line not a big deal though
    // first lets work only on parsing for now, and then use syntax directed translation to guide the parser
    Token next_token = TokenizerLookahead(parser->tokenizer);
    // f(x) = x

    switch (next_token.type)
    {
    case TOKEN_NONE:
        return;
    case TOKEN_ID:
    {
        // Not enough information from here
        // Parse it as a variable now
        Assert(ParseVar(&parser->tokenizer));
    }
    break;
    }
}

SymbolFn *CreateSymbolFn(uint32_t args_max)
{
    SymbolFn *fn = malloc(sizeof(*fn));
    return fn;
}

// TODO :: Implement scope checking at the site of definition
bool ParseFuncBody(Parser *parser, SymbolTable *symbol_table, SymbolFn *fn)
{
    // It will start right after consuming equal token
    // The scope that should be currently use is the one created by the function's arg parameters

    // Create a new symbol table with variables only
    // Do not evaluate the variables used in this symbol table

    SymbolTable *table = CreateSymbolTable(false);

    for (uint32_t arg = 0; arg < fn->args_count; ++arg)
    {
        table->variables[table->var_count] = fn->args[arg];
        table->var_count                   = table->var_count + 1;
    }

    PushToSymbolTableStack(&symbol_table_stack, table);
    fn->expr_tree = CreateExprTree(parser);
    PopFromSymbolTableStack(&symbol_table_stack);
    // TODO :: Wrap it
    free(table);
    return true;
    // If the expr being evaluated is function and then the variable that is in scope shouldn't be dealt with.
    // So CreateExprTree() function should behave differently to parsing function and variables
    // And thats normally expected behavior too
    // Need to think of another alternative approach
    // Infuse information directly in the symbol table
}

bool ParseVar(Parser *parser)
{
    Token token = TokenizeNext(parser->tokenizer);
    Token next  = TokenizerLookahead(parser->tokenizer);

    if (next.type == TOKEN_OPAREN)
    {
        // start of the function parsing phase
        // parse function here recursively
        // start of the function declaration phase

        // Simply loop through the tokens skipping commas and add to function declaration

        SymbolFn *fn = CreateSymbolFn(0);
        memset(fn, 0, sizeof(*fn));
        Assert(fn != NULL);
        strcpy(fn->id, token.token_id.name);

        TokenizeNext(parser->tokenizer);
        token = TokenizeNext(parser->tokenizer);

        while (token.type != TOKEN_CPAREN)
        {
            // Its just a token id
            NamedAssert(token.type == TOKEN_ID, token.type);
            // The args only contains the variable
            fn->args[fn->args_count].var_type = VAR_ID;
            strcpy(fn->args[fn->args_count].data.id, token.token_id.name);

            fn->args_count = fn->args_count + 1;
            token          = TokenizeNext(parser->tokenizer);

            Assert(token.type == TOKEN_COMMA || token.type == TOKEN_CPAREN);
            if (token.type == TOKEN_CPAREN)
                break;
            token = TokenizeNext(parser->tokenizer);
        }

        // The parsing stops right after consuming the closing paretheses token

        parser->current_token = TokenizeNext(parser->tokenizer);
        // Parse the function body from here onward
        NamedAssert(parser->current_token.type == TOKEN_EQUAL, (int)parser->current_token.type);

        // Implicity creates a new scope with variables x and y as the parameters
        SymbolTable *top = TopOfSymbolTableStack(&symbol_table_stack);
        ParseFuncBody(parser, top, fn);

        InsertSymbolFn(top, fn);
        return true;
    }
    else if (next.type == TOKEN_EQUAL)
    {
        // its a variable, add it to the symbol entry
        SymbolVar var;
        strcpy(var.data.id, token.token_id.name); // ignore length for now

        TokenizeNext(parser->tokenizer);

        SymbolTable *top       = TopOfSymbolTableStack(&symbol_table_stack);

        ExprTree    *expr_tree = CreateExprTree(parser);
        var.var_type           = VAR_VALUE;
        // I guess, stack doesn't need to be provided here
        var.data.value = EvalExprTreeWithSymbolTable(top, expr_tree);
        // var.data.value = EvalExprTree(expr_tree);
        DestroyExprTree(expr_tree);
        // parser->current_token = TokenizeNext(parser->tokenizer);    // skip the lookahead
        // ParserVarBody(parser, &symbol);
        InsertSymbolVar(top, &var);
        return true;
    }
    return false;
}

void PrintSymbolTable(SymbolTable *symbol_table)
{
    fprintf(stdout, "\n \t\t\t Symbol Table Output");
    fprintf(stdout, "\n \t\t\t Variables \n");

    fprintf(stdout, "Variables count : %u.\n\n", symbol_table->var_count);
    for (uint32_t var = 0; var < symbol_table->var_count; ++var)
    {
        SymbolVar *entry = &symbol_table->variables[var];
        fprintf(stdout, "Var   : %-20s \nValue : %-20u\n", entry->data.id, entry->data.value);
    }

    fprintf(stdout, "\n\t\t\t Functions \n");
    fprintf(stdout, "Functions count : %u.\n\n", symbol_table->fn_count);

    for (uint32_t fn = 0; fn < symbol_table->fn_count; ++fn)
    {
        SymbolFn *entry = symbol_table->functions[fn];
        fprintf(stdout, "Function : %s(", entry->id);

        for (int32_t arg = 0; arg < (int32_t)entry->args_count - 1; ++arg)
        {
            fprintf(stdout, "%s, ", entry->args[arg].data.id);
        }
        if (entry->args_count)
            fprintf(stdout, "%s", entry->args[entry->args_count - 1].data.id);
        fprintf(stdout, ")");
    }
}

void RunInterpreter(Parser *parser)
{
    // only handle one line at each step
    uint32_t total_len = parser->tokenizer->buffer.len;
    uint32_t pos       = 0;

    while (pos < total_len)
    {
        // scan forward till new line found
        uint32_t nlen = total_len - pos;
        for (uint32_t npos = pos; npos < total_len; ++npos)
        {
            if (parser->tokenizer->buffer.data[npos] == '\n')
            {
                nlen = npos - pos;
                break;
            }
        }
        parser->tokenizer->buffer.pos = pos;
        parser->tokenizer->buffer.len = nlen + pos;
        ParseStart(parser);
        pos = pos + nlen + 1;
    }
}

void EvalAndPrintFunctions(SymbolFn *fn)
{
    SymbolTable *table = CreateSymbolTable(true);
    // Populate this symbol table with the arguments of functions
    for (uint32_t arg = 0; arg < fn->args_count; ++arg)
    {
        table->variables[table->var_count] = fn->args[arg];
        table->variables[table->var_count].var_type = TERM_VALUE; 
        table->var_count                   = table->var_count + 1;
    }
    // find x 
    SymbolVar *var_x = FindSymbolTableEntryVar(table,"x"); 
    Assert(var_x != NULL); 
    SymbolVar *var_y = FindSymbolTableEntryVar(table, "y"); 
    Assert(var_y != NULL); 

    for (uint32_t x = 0; x < 10; ++x)
    {
        var_x->data.value = x; 
        for (uint32_t y = 0; y < 10; ++y)
        {
            var_y->data.value = y; 
            fprintf(stdout, "(%2u,%2u) -> %2u |",x,y, EvalExprTreeWithSymbolTable(table,fn->expr_tree));
        }
        fprintf(stdout, "\n");
    }
    free(table); 
}

int main(int argc, char **argv)
{
    InitSymbolTableStack(&symbol_table_stack);
    // const char *string = "1 + 2 * (3 * 4 + 44 / 4 * 2) / 4 - 3 ";
    // const char *string    = "2 * 2";
    // Tokenizer  *tokenizer = CreateTokenizer(string, strlen(string));
    //// Token       tok       = TokenizeNext(tokenizer);
    //// PrintToken(&tok);
    // Parser parser;
    // parser.tokenizer = tokenizer;
    // ExprTree *expr1  = CreateExprTree(&parser);
    // fprintf(stderr, "Output : %u.", EvalExprTree(expr1));

    // Symbol table is currently global
    // InitSymbolTable(&symbol_table);
    // const char *expr          = "a = 4 + 2 * 50";
    // Tokenizer  *new_tokenizer = CreateTokenizer(expr, strlen(expr));
    // Parser      parser2       = {.tokenizer = new_tokenizer};
    // ParseStart(&parser2);

    //// Ready for next session
    //// const char *expr2 = "b = a * 4 + 3 * 2 - a * 3";
    // const char *expr2 = "f(x,y) = x + y";
    // TokenizerSetBuffer(parser2.tokenizer, expr2, strlen(expr2));
    // ParseStart(&parser2);
    // PrintSymbolTable(&symbol_table);

    const char *expr    = "a = 4 \n b = 5 \n c = a + 2 * b \n cd = a * a + b * b \n e(x,y) = x * y";
    Parser      nparser = {.tokenizer = CreateTokenizer(expr, strlen(expr))};
    RunInterpreter(&nparser);

    SymbolTable *scope = CheckVarInScope(&symbol_table_stack, "cd");
    Assert(scope != NULL);

    // We know have functions, now we need a way to evaluate it
    PrintSymbolTable(symbol_table_stack.symbol_tables[0]);

    fprintf(stdout, "Showing function implementation test \n"); 
    for (uint32_t fn = 0; fn < symbol_table_stack.symbol_tables[0]->fn_count; ++fn)
        EvalAndPrintFunctions(symbol_table_stack.symbol_tables[0]->functions[fn]); 
    return 0;
}