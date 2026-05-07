#include "colors.h"
#include "ncc.h"
#include <stdio.h>


typedef enum symtbl_env_t	// enviroment of variables
{
	SYMTBL_ENV_FUNCARG,
	SYMTBL_ENV_LOCAL
} symtbl_env_t;

static alerts_t ALERTS = {};

/*-------------------------------------------*/
static symtbl_t *TblInit(void);
static ssize_t TblAddVar(const char *name, symtbl_t *symtbl, const symtbl_env_t vartype);

static ssize_t TblGetID(const char *name, symtbl_t *symtbl, const symtbl_env_t vartype);

static void TblDestroy(symtbl_t *symtbl);
static node_t *NewBinNode(const node_data_t data, node_t *l_val, node_t *r_val);

static node_t *GetOp(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetDeclFunc(node_data_t *data[]);
static node_t *GetCallFunc(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetReturn(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetSmplKword(node_data_t *data[], const node_data_t kword);	/* simple keyword */
static node_t *GetWhileIf(node_data_t *data[], symtbl_t *symtbl, const node_data_t while_or_if);
static node_t *GetLiteral(node_data_t *data[]);
static node_t *GetAsm(node_data_t *data[]);
static node_t *GetAssign(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetOrExpr(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetAndExpr(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetCompExpr(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetExpr(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetTemp(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetPrim(node_data_t *data[], symtbl_t *symtbl);

static node_t *GetVarExpr(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetVarAdr(node_data_t *data[], symtbl_t *symtbl);
static node_t *GetDeref(node_data_t *data[], symtbl_t *symtbl);

#define __GetVar(data, symtbl, vartype, ...)	_GetVar(data, symtbl, vartype)
static node_t *_GetVar(node_data_t *data[], symtbl_t *symtbl, const symtbl_env_t vartype);
#define GetVar(...)				__GetVar(__VA_ARGS__, SYMTBL_ENV_LOCAL)

static node_t *GetNum(node_data_t *data[]);
/*-------------------------------------------*/


static symtbl_t *TblInit(void)
{
	symtbl_t *symtbl = (symtbl_t *)calloc(1, sizeof(symtbl_t));
	assert(symtbl);

	return symtbl;
}

static ssize_t TblAddVar(const char *name, symtbl_t *symtbl, const symtbl_env_t vartype)
{
	assert(name);
	assert(symtbl);
	
	if(symtbl->size + 1 >= symtbl->cap)
	{
		symtbl->cap = 2 * (symtbl->size + 1);
		symtbl->cell = (symtbl_cell_t *)reallocarray(symtbl->cell, symtbl->cap, sizeof(symtbl_cell_t));
		if(symtbl->cell == NULL)
		{
			print_err_msg("nametable overflow");
			abort();
		}
	}

	symtbl->cell[symtbl->size].name = name;

	// if crnt var isn't a function's arg
	if(vartype == SYMTBL_ENV_LOCAL)
	{
			// if tbl isn't empty and prev var wasn't function's arg
		if(symtbl->size && symtbl->cell[symtbl->size-1].id < 0)
			symtbl->cell[symtbl->size].id = symtbl->cell[symtbl->size-1].id - 1;
		else	// otherwise (empty tbl or prev var was arg)
			symtbl->cell[symtbl->size].id = -1;	// init val
	}
	else	// if crnt var is a func's arg
	{
		if(symtbl->size)
			symtbl->cell[symtbl->size].id = symtbl->cell[symtbl->size-1].id + 1;
		else
			symtbl->cell[symtbl->size].id = 2;	// init val
	}

	return symtbl->cell[symtbl->size++].id;
}

static ssize_t TblGetID(const char *name, symtbl_t *symtbl, const symtbl_env_t vartype)
{
	assert(name);	assert(symtbl);

	for (size_t i = 0; i < symtbl->size; i++)
		if(strcmp(name, symtbl->cell[i].name) == 0)
			return symtbl->cell[i].id;

	return TblAddVar(name, symtbl, vartype);
}

static void TblDestroy(symtbl_t *symtbl)
{
	if(symtbl == NULL)
		return;

	free(symtbl->cell);
	symtbl->cell = NULL;
	symtbl->size = symtbl->cap = 0;

	free(symtbl);
}

static node_t *NewBinNode(const node_data_t data, node_t *l_val, node_t *r_val)
{
	node_t *eq = NewNode(data);
	AddChild(eq, l_val);
	AddChild(eq, r_val);

	return eq;
}

static int PrintAlerts(const char *filename)	/* returns 1 if compilation errors occured */
{
	assert(filename);

	int has_err = 0;

	for (size_t i = 0; i < ALERTS.n_alert; i++)
	{
		switch(ALERTS.alert[i].type)
		{
		case AL_NOTICE:
			fprintf(stderr, colorize("notice: ", _BOLD_ _CYAN_));
			break;
		case AL_WARNING:
			fprintf(stderr, colorize("warning: ", _BOLD_ _MAGENTA_));
			break;
		case AL_ERROR:
			fprintf(stderr, colorize("error: ", _BOLD_ _RED_));
			has_err = 1;
			break;
		default:
			break;
		}

		fprintf(stderr, colorize("%s:%lu:\n", _BOLD_ _YELLOW_) colorize("%s\n", _BOLD_ _WHITE_), filename, ALERTS.alert[i].line, ALERTS.alert[i].msg);
	}

	return has_err;
}
/*-------------------------------------------*/
#include "def_macro.h"
/*-------------------------------------------*/
node_t *Parse(toks_t *toks, const char *filename)
{
	if(toks == NULL || filename == NULL)
	{
		print_err_msg("nullptr passed as arg(s)");
		return NULL;
	}

	node_t *node = NewNode(ROOT);

	node_data_t *data = toks->data;
	// node_t *new_node = GetDeclFunc(&data);
	node_t *new_node = NULL;
	// if(new_node == NULL)
	// {
	// 	print_err_msg("There aren't any functions");
	// 	TreeDestroy(node);
	// 	return NULL;
	// }

	while((new_node = GetDeclFunc(&data)))
		AddChild(node, new_node);

	if(data->type != TP_EOF)
		write_err("is not a function declaration", data->line);

	if(PrintAlerts(filename))
	{
		TreeDestroy(node);
		node = NULL;
	}
	
	return node;
}

static node_t *GetOp(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL;
	node_t *new_node = NULL;

	if(IS_(OPN_BRC, **data))
	{
		(*data)++;
		node = NewNode(OP_SEQ);

		new_node = GetOp(data, symtbl);
		if(new_node == NULL)	/* in general, it isn't necessary... */
			write_err("excepted operation(s)", (**data).line);

		while(new_node)
		{
			AddChild(node, new_node);
			new_node = GetOp(data, symtbl);
		}
		
		if(IS_(CLS_BRC, **data))
			(*data)++;
		else
			write_err("missing '}'", (**data).line);
	}
	else if((node = GetWhileIf(data, symtbl, IF))	||
			(node = GetWhileIf(data, symtbl, WHILE)));
	else if((node = GetAsm(data))				||
			(node = GetReturn(data, symtbl))	||
			(node = GetCallFunc(data, symtbl))	||
			(node = GetAssign(data, symtbl))	||
			(node = GetSmplKword(data, PASS))	||
			(node = GetSmplKword(data, BREAK))	||
			(node = GetSmplKword(data, CONTINUE)))
	{
		if(IS_(SEMICOLON, **data))
			(*data)++;
		else
			write_err("missing ';'", (**data).line);
	}

	return node;
}

static node_t *GetDeclFunc(node_data_t *data[])
{
	assert(data);
	assert(*data);

	node_t *node = NULL, *arg_node = NULL;
	symtbl_t *symtbl = NULL;

	if (IS_(FUNC, **data))
	{
		(*data)++;
		if((**data).type == TP_IDENT && IS_(OPN_PAR, (*data)[1]))
		{
			node = NewNode(FUNC_DECL((**data).val.name));
			(*data) += 2;
			AddChild(node, NewNode(PARAM));
			
			symtbl = TblInit();
			
			while((**data).type == TP_IDENT)	/* arguments */
			{
				arg_node = GetVar(data, symtbl, SYMTBL_ENV_FUNCARG);
				if(arg_node)
					AddChild(node->child->node, arg_node);
				else
					write_err("invalid function's arguments", (**data).line);

				if(IS_(COMMA, **data))
					(*data)++;
				else
					break;
			}
			
			if (IS_(CLS_PAR, **data))
				(*data)++;
			else
				write_err("is it function declaration? excepted ',' or ')'", (**data).line);
			
			node->child->node->data.val.id = (ssize_t)symtbl->size;

			if((arg_node = GetOp(data, symtbl)))
				AddChild(node, arg_node);
			else
				write_err("excepted function's body", (**data).line);

			node->child->next->node->data.val.id = (ssize_t)symtbl->size - node->child->node->data.val.id;
		}
		else
			write_err("function's signature missing", (**data).line);
	}

	TblDestroy(symtbl);
	return node;
}

static node_t *GetCallFunc(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL, *arg_node = NULL;

	if((**data).type == TP_IDENT && IS_(OPN_PAR, (*data)[1]))
	{
		node = NewNode(FUNC_CALL((**data).val.name));
		AddChild(node, NewNode(PARAM));
		(*data) += 2;

		ssize_t param_count = 0;
		while ((arg_node = GetOrExpr(data, symtbl)))
		{
			param_count++;
			AddChild(node->child->node, arg_node);
			if(IS_(COMMA, **data))
				(*data)++;
			else
				break;
		}
		if(IS_(CLS_PAR, **data))
			(*data)++;
		else
			write_err("is it function call? excepted ',' or ')'", (**data).line);

		node->child->node->data.val.id = param_count;
	}
	else
		return NULL;

	return node;
}

static node_t *GetReturn(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL, *arg_node = NULL;

	if(IS_(RETURN, **data))
	{
		node = NewNode(RETURN);
		(*data)++;

		if(!IS_(SEMICOLON, **data))
		{
			arg_node = GetOrExpr(data, symtbl);
			if(arg_node)
				AddChild(node, arg_node);
			else
				write_err("excepted ';'", (**data).line);
		}
	}

	return node;
}

static node_t *GetSmplKword(node_data_t *data[], const node_data_t kword)
{
	assert(data);
	assert(*data);

	node_t *node = NULL;

	if(IS_(kword, **data))
	{
		node = NewNode(kword);
		(*data)++;
	}

	return node;
}

static node_t *GetWhileIf(node_data_t *data[], symtbl_t *symtbl, const node_data_t while_or_if)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL;
	node_t *new_node = NULL;

	if(IS_(while_or_if, **data))
	{
		(*data)++;
		
		node = NewNode(while_or_if);
		
		if(IS_(OPN_PAR, **data))
		{
			(*data)++;
			
			if((new_node = GetOrExpr(data, symtbl)))
			{
				AddChild(node, new_node);
				
				if(IS_(CLS_PAR, **data))
					(*data)++;
				else
					write_err("missing ')'", (**data).line);

				if((new_node = GetOp(data, symtbl)))
					AddChild(node, new_node);
				else
					write_err("excepted operations body", (**data).line);
				
				if(IS_(ELSE, **data))
				{
					(*data)++;
					
					if((new_node = GetOp(data, symtbl)))
						AddChild(node, new_node);
					else
						write_err("excepted 'else' operations body", (**data).line);
				}
			}
			else
				write_err("wrong condition", (**data).line);
		}
		else
			write_err("missing '('", (**data).line);
	}

	return node;
}

static node_t *GetLiteral(node_data_t *data[])
{
	assert(data);	assert(*data);

	node_t *node = NULL;

	if((**data).type == TP_LITERAL)
	{
		node = NewNode(**data);
		(*data)++;
	}

	return node;
}

static node_t *GetAsm(node_data_t *data[])
{
	assert(data);
	assert(*data);

	node_t *node = NULL, *literal_node = NULL;

	if (IS_(ASM, **data))
	{
		(*data)++;
		
		if(IS_(OPN_PAR, **data))
		{
			(*data)++;
			
			if((literal_node = GetLiteral(data)))
			{
				node = NewNode(ASM);
				AddChild(node, literal_node);
			}
			else
				write_err("excepted a literal", (**data).line);
		}
		else
			write_err("missing '('", (**data).line);

		if(IS_(CLS_PAR, **data))
			(*data)++;
		else
			write_err("missing ')'", (**data).line);
	}

	return node;
}

static node_t *GetAssign(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL;
	node_t *new_node = NULL;

	if((new_node = GetVarExpr(data, symtbl)))
	{
		if(IS_(ASSIGN, **data))
		{
			(*data)++;

			node = NewNode(ASSIGN);
			AddChild(node, new_node);

			if((new_node = GetOrExpr(data, symtbl)))
				AddChild(node, new_node);
			else
				write_err("missing rvalue", (**data).line);
		}
		else
			TreeDestroy(new_node);
	}

	return node;
}

static node_t *GetOrExpr(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = GetAndExpr(data, symtbl), *new_node = NULL, *arg_node = NULL;
	if(node == NULL)
		return NULL;
	
	while(IS_(OR, **data))
	{
		(*data)++;

		if((arg_node = GetAndExpr(data, symtbl)))
		{
			new_node = NewBinNode(OR, node, arg_node);
			node = new_node;
		}
		else
			write_err("missing expression (right from 'or')", (**data).line);
	}

	return node;
}

static node_t *GetAndExpr(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = GetCompExpr(data, symtbl), *new_node = NULL, *arg_node = NULL;
	if(node == NULL)
		return NULL;
	
	while(IS_(AND, **data))
	{
		(*data)++;

		if((arg_node = GetCompExpr(data, symtbl)))
		{
			new_node = NewBinNode(AND, node, arg_node);
			node = new_node;
		}
		else
			write_err("missing expression (right from 'and')", (**data).line);
	}

	return node;
}

static node_t *GetCompExpr(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = GetExpr(data, symtbl), *arg_node = NULL;
	if(node == NULL)
		return NULL;

	if(IS_(GREATER, **data) || IS_(LESS, **data) || IS_(EQ, **data) || IS_(NEQ, **data))
	{
		node_data_t op = **data;
		(*data)++;

		if((arg_node = GetExpr(data, symtbl)))
			node = NewBinNode(op, node, arg_node);
		else
			write_err("missing expression (right from compare)", (**data).line);
	}

	return node;
}

static node_t *GetExpr(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *new_node = NULL, *arg_node = NULL;
	node_t *node = GetTemp(data, symtbl);
	
	while(IS_(ADD, **data) || IS_(SUB, **data))
	{
		new_node = NewNode(**data);
		(*data)++;
		
		if(node == NULL)
		{
			write_wrg("ncc 1.0 forbids unary operators", (**data).line);
			node = NewNode(NUM(0));
		}		
		AddChild(new_node, node);
		
		if((arg_node = GetTemp(data, symtbl)))
			AddChild(new_node, arg_node);
		else
			write_err("missing right expression (from '+' or '-')", (**data).line);

		node = new_node;
	}

	return node;
}

static node_t *GetTemp(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *new_node = NULL, *arg_node = NULL;
	node_t *node = GetPrim(data, symtbl);
	if(node == NULL)
		return NULL;

	while(IS_(MUL, **data) || IS_(DIV, **data) || IS_(MOD, **data))
	{
		new_node = NewNode(**data);
		(*data)++;

		AddChild(new_node, node);
		
		if((arg_node = GetPrim(data, symtbl)))
			AddChild(new_node, arg_node);
		else
			write_err("missing right expression (from '*', '/', or '%')", (**data).line);

		node = new_node;
	}

	return node;
}

static node_t *GetPrim(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL;

	if(IS_(OPN_PAR, **data))
	{
		(*data)++;
		if((node = GetOrExpr(data, symtbl)))
		{
			if(IS_(CLS_PAR, **data))
				(*data)++;
			else
				write_err("missing ')'", (**data).line);
		}
		else
			write_err("missing expression", (**data).line);
	}
	else if((node = GetCallFunc(data, symtbl)));
	else if((node = GetLiteral(data)));
	else if((node = GetNum(data)));
	else	node = GetVarExpr(data, symtbl);

	return node;
}


static node_t *GetVarExpr(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL;
	
	if((node = GetVarAdr(data, symtbl)));
	else if((node = GetDeref(data, symtbl)));
	else
		node = GetVar(data, symtbl);

	return node;
}

static node_t *GetVarAdr(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL;

	if((**data).type == TP_TAKEADDR)
	{
		(*data)++;

		if ((**data).type == TP_IDENT)
		{
			node = NewNode(TAKEADDR(TblGetID((**data).val.name, symtbl, SYMTBL_ENV_LOCAL)));

			(*data)++;
		}
		else
			write_err("excepted variable after '&'", (**data).line);
	}

	return node;
}

static node_t *GetDeref(node_data_t *data[], symtbl_t *symtbl)
{
	assert(data);
	assert(*data);
	assert(symtbl);

	node_t *node = NULL, *op_node = NULL, *bias_var = NULL;

	if((**data).type == TP_IDENT && IS_(OPN_BRK, (*data)[1]))
		bias_var = GetVar(data, symtbl);

	if(IS_(OPN_BRK, **data))
	{
		(*data)++;

		if((op_node = GetOrExpr(data, symtbl)))
		{
			node = NewNode(DEREF);

			if(bias_var)	op_node = NewBinNode(ADD, bias_var, op_node);

			AddChild(node, op_node);				// var to deref
		}
		else
		{
			write_err("excepted expression in '[]'", (**data).line);
			free(bias_var);
		}
		
		if(IS_(CLS_BRK, **data))
			(*data)++;
		else
			write_err("missing ']'", (**data).line);
	}

	return node;
}

static node_t *GetNum(node_data_t *data[])
{
	assert(data);
	assert(*data);
	
	if((**data).type == TP_NUM)
	{
		node_t *node = NewNode(**data);
		(*data)++;
		return node;
	}

	return NULL;
}

static node_t *_GetVar(node_data_t *data[], symtbl_t *symtbl, const symtbl_env_t vartype)
{
	assert(data);
	assert(*data);
	
	if((**data).type == TP_IDENT)
	{
		node_t *node = NewNode(VAR(TblGetID((**data).val.name, symtbl, vartype)));
		
		(*data)++;
		return node;
	}

	return NULL;
}
/*-------------------------------------------*/
#include "undef_macro.h"
