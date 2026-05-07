#include "colors.h"
#include "ncc.h"
#include <malloc.h>

static size_t CRNT_LINE = 1;	/* global lines counter */
	
/*-------------------------------------------*/
static void TokRealloc(toks_t *toks);
static char ReadChar(const char **s);
static int SkipComments(const char **s);
static int Lexem(toks_t *toks, const char **s);
static int Number(toks_t *toks, const char **s);
static int Ident(toks_t *toks, const char **s);
static int Literal(toks_t *toks, const char **s);
/*-------------------------------------------*/

toks_t *Tokenize(const char *s)
{
	assert(s);

	toks_t *toks = (toks_t *)calloc(1, sizeof(toks_t));
	assert(toks);

	while(*s > 0)
	{
		TokRealloc(toks);

		if (SkipComments(&s));
		else if(Lexem(toks, &s));
		else if(Ident(toks, &s));
		else if(Number(toks, &s));
		else if(Literal(toks, &s));
		else
		{
			print_err_msg("syntax error");
			print_wrong_s(s);
			ToksDestroy(toks);
			return NULL;
		}
	}
	TokRealloc(toks);
	toks->data[toks->size++] = (const node_data_t){.type = TP_EOF};

	return toks;
}

static void TokRealloc(toks_t *toks)
{
	assert(toks);
	
	if(toks->size + 1 >= toks->cap)
	{
		toks->cap = 2 * (toks->size + 1);
		toks->data = (node_data_t *)reallocarray(toks->data, toks->cap, sizeof(node_data_t));
		if(toks->data == NULL)
		{
			print_err_msg("tokens overflow");
			abort();
		}
	}
}

static char ReadChar(const char **s)
{
	assert(s);	assert(*s);

	char c = 0;

	if(**s == '\\')
	{
		(*s)++;

		switch(**s)
		{
			case '0':	c = '\0'; break;
			case 'a':	c = '\a'; break;
			case 'b':	c = '\b'; break;
			case 't':	c = '\t'; break;
			case 'n':	c = '\n'; break;
			case 'v':	c = '\v'; break;
			case 'f':	c = '\f'; break;
			case 'r':	c = '\r'; break;

			case '\'':	c = '\''; break;
			case '\\':	c = '\\'; break;
			case '\"':	c = '\"'; break;

			default:
					print_err_msg("invalid \\-code");
					return 0;
		}
	}
	else	c = **s;

	(*s)++;	return c;
}

static int SkipComments(const char **s)
{
	assert(s);
	assert(*s);

	const char *old_s = *s;
	
	while (isspace(**s))
	{
		if(**s == '\n')
			CRNT_LINE++;

		(*s)++;
	}

	if(strncmp(*s, "/*", 2) == 0)
	{
		(*s) += 2;
		while(**s && strncmp(*s, "*/", 2))
		{
			if(**s == '\n')
				CRNT_LINE++;

			(*s)++;
		}
		
		if(**s)
			(*s) += 2;
		else
			return 0;
	}

	if(*s == old_s)
		return 0;

	return 1;
}

static int Lexem(toks_t *toks, const char **s)
{
	assert(toks);
	assert(s);
	assert(*s);

	for (size_t i = 0; i < N_LEXS; i++)
	{
		if(strncmp(LEXS[i].name, *s, strlen(LEXS[i].name)) == 0)
		{
			toks->data[toks->size] = LEXS[i].data;
			toks->data[toks->size].line = CRNT_LINE;
			toks->size++;
			(*s) += strlen(LEXS[i].name);
			return 1;
		}
	}

	return 0;
}

#include "def_macro.h"
static int Number(toks_t *toks, const char **s)
{
	assert(toks);
	assert(s);
	assert(*s);

	long num = 0;

	if(**s == '\'')
	{
		(*s)++;

		num = ReadChar(s);

		if(**s != '\'')
		{
			print_err_msg("missing '");
			return 0;
		}

		(*s)++;
	}
	else
	{
		char *end_s = NULL;
		num = strtol(*s, &end_s, 10);
		assert(end_s);

		if(isalpha(*end_s) || *end_s == '_')	/* letter or '_' can't follow after number */
			return 0;

		if(end_s == *s)
			return 0;

		*s = end_s;
	}

	toks->data[toks->size] = NUM(num);
	toks->data[toks->size].line = CRNT_LINE;
	toks->size++;

	return 1;
}
#include "undef_macro.h"

static int Ident(toks_t *toks, const char **s)
{
	assert(toks);
	assert(s);
	assert(*s);
	
	if(isdigit(**s))
		return 0;

	char *name = NULL;
	int name_len = 0;
	if (sscanf(*s, "%m[A-Za-z0-9_]%n", &name, &name_len) > 0)
	{
		toks->data[toks->size].type = TP_IDENT;
		toks->data[toks->size].val.name = name;
		toks->data[toks->size].line = CRNT_LINE;
		toks->size++;
		
		(*s) += name_len;

		return 1;
	}

	free(name);
	return 0;
}

static int Literal(toks_t *toks, const char **s)
{
	assert(toks);
	assert(s);
	assert(*s);
	
	if(**s == '"')
	{
		(*s)++;

		size_t len = 0, cap = 10;
		char *str = (char *)calloc(cap, sizeof(char));	assert(str);

		while(**s != '"' && **s != '\0')
		{
			str[len++] = ReadChar(s);

			if(len >= cap)
			{
				str = reallocarray(str, (cap *= 2), sizeof(char));
				assert(str);
			}	
		}
		if(**s != '"')
		{
			print_err_msg("missing '\"'");
			free(str);	return 0;
		}

		str[len] = '\0';
		toks->data[toks->size].type = TP_LITERAL;
		toks->data[toks->size].val.name = str;
		toks->size++;

		(*s)++;	return 1;
	}

	return 0;
}

void ToksDestroy(toks_t *toks)
{
	if(toks == NULL)
		return;

	for (size_t i = 0; i < toks->size; i++)
	{
		if(toks->data[i].type == TP_IDENT || toks->data[i].type == TP_LITERAL)
		{
			free(toks->data[i].val.name);
			toks->data[i].val.name = NULL;
		}
	}

	free(toks->data);
	toks->data = NULL;
	toks->size = toks->cap = 0;
	free(toks); /* dubiously, but ok */
}

