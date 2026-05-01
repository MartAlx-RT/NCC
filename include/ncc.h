#ifndef NCC_H
#define NCC_H

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include "colors.h"

typedef enum node_type_t
{
	TP_EOF,
	TP_ROOT,
	TP_NUM,
	TP_OP,
	TP_OP_SEQ,
	TP_IDENT,
	TP_PARAM,
	TP_VAR,
	TP_KWORD,
	TP_SYMB,
	TP_DECL_FUNC,
	TP_CALL_FUNC,
	TP_LITERAL,
	TP_TAKEADDR,
	TP_DEREF,
} node_type_t;

typedef enum op_t
{
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_GREATER,
	OP_LESS,
	OP_ASSIGN,
	OP_EQ,
	OP_OR,
	OP_AND,
} op_t;

typedef enum kword_t
{
	KW_IF,
	
	KW_ELSE,
	
	KW_WHILE,
	KW_FOR,
	
	KW_CONTINUE,
	KW_BREAK,
	
	KW_RETURN,
	KW_PASS,
	
	KW_ASM,
	KW_FUNC,
} kword_t;

typedef enum symb_t
{
	SYM_OPN_BRC,
	SYM_CLS_BRC,
	SYM_OPN_PAR,
	SYM_CLS_PAR,
	SYM_SEMICOL,
	SYM_COMMA,
	SYM_QUOTE,
	SYM_OPN_BRK,
	SYM_CLS_BRK,
} symb_t;

typedef union node_val_t
{
	char *name;
	ssize_t id;
	long num;
	op_t op;
	symb_t symb;
	kword_t kword;

} node_val_t;

typedef struct node_data_t
{
	node_type_t type;
	node_val_t val;
	size_t line;	/* location token in code */
} node_data_t;

typedef struct node_t
{
	node_data_t data;
	struct node_t *parent;
	struct child_t *child;
} node_t;

typedef struct child_t
{
	node_t *node;
	struct child_t *prev, *next;	// prev[first_child] -> last_child
	
} child_t;

typedef struct lex_t
{
	const char *name;
	const node_data_t data;
} lex_t;

typedef struct toks_t
{
	node_data_t *data;
	size_t size;
	size_t cap;

} toks_t;

#include "def_macro.h"

static const lex_t LEXS[] =
	{
		{"{", OPN_BRC},
		{"}", CLS_BRC},
		{"(", OPN_PAR},
		{")", CLS_PAR},
		{";", SEMICOLON},
		{",", COMMA},
		{"&", TAKEADDR(0)},
		{"[",OPN_BRK},
		{"]",CLS_BRK},
		{"==", EQ},
		{"=", ASSIGN},
		{"+", ADD},
		{"-", SUB},
		{">", GREATER},
		{"<", LESS},
		{"*", MUL},
		{"/", DIV},
		{"or", OR},
		{"and", AND},
		
		{"if", IF},
		{"else", ELSE},
		{"while", WHILE},
		{"for", FOR},
		{"asm", ASM},
		{"func", FUNC},
		{"return", RETURN},
		{"pass", PASS},
		{"break", BREAK},
		{"continue", CONTINUE}
};

#include "undef_macro.h"

typedef enum tree_err_t
{
	TR_NO_ERR,
	TR_EXT_ERR,
	TR_NULLPTR,
	TR_OVERFLOW,

} tree_err_t;

typedef struct symtbl_cell_t
{
	const char *name;
	ssize_t id;
} symtbl_cell_t;

typedef struct symtbl_t
{
	symtbl_cell_t *cell;
	size_t size;
	size_t cap;
} symtbl_t;

#define N_ALERT_LIMIT 50

typedef enum alert_type_t
{
	AL_NOTICE,
	AL_WARNING,
	AL_ERROR
} alert_type_t;

typedef struct alert_t
{
	alert_type_t type;
	const char *msg;
	size_t line;
} alert_t;

typedef struct alerts_t
{
	alert_t alert[N_ALERT_LIMIT];
	size_t n_alert;
} alerts_t;

static const size_t N_LEXS = sizeof(LEXS) / sizeof(lex_t);
static const size_t MAX_REC_DEPTH = 1000;

/*--------------------------------------*/

/* input */
size_t ReadFileToBuf(const char *file_path, char **buf);

/* dump */
void PrintToks(node_data_t data[], FILE *dump_file, size_t limit);
void TreeDump(const node_t *tree);

/* tree functions */
void AddChild(node_t *node, node_t *new_child);
void TreeDestroy(node_t *tree);
node_t *NewNode(const node_data_t data);

/* front-end */
toks_t *Tokenize(const char *s);
node_t *Parse(toks_t *toks, const char *filename);
void ToksDestroy(toks_t *toks);

/* back-end */
int CompileTree(const node_t *tree, FILE *asm_out);

#endif
