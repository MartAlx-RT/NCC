#include "ncc.h"


static alerts_t ALERTS = {};

/*-------------------------------------------*/
static nametbl_t *TblInit(void);
static size_t TblAddName(const char *name, nametbl_t *nametbl);
static size_t TblGetID(const char *name, nametbl_t *nametbl);
static void TblDestroy(nametbl_t *nametbl);
static node_t *NewBinNode(const node_data_t data, node_t *l_val, node_t *r_val);

static node_t *GetOp(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetDeclFunc(node_data_t *data[]);
static node_t *GetCallFunc(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetReturn(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetSmplKword(node_data_t *data[], const node_data_t kword);	/* simple keyword */
static node_t *GetWhileIf(node_data_t *data[], nametbl_t *nametbl, const node_data_t while_or_if);
static node_t *GetAsm(node_data_t *data[]);
static node_t *GetAssign(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetOrExpr(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetAndExpr(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetCompExpr(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetExpr(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetTemp(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetPrim(node_data_t *data[], nametbl_t *nametbl);

static node_t *GetVarExpr(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetVarAdr(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetDeref(node_data_t *data[], nametbl_t *nametbl);

static node_t *GetVar(node_data_t *data[], nametbl_t *nametbl);
static node_t *GetNum(node_data_t *data[]);
/*-------------------------------------------*/
static nametbl_t *TblInit(void)
{
	nametbl_t *nametbl = (nametbl_t *)calloc(1, sizeof(nametbl_t));
	assert(nametbl);

	return nametbl;
}

static size_t TblAddName(const char *name, nametbl_t *nametbl)
{
	assert(name);
	assert(nametbl);
	
	if(nametbl->size + 1 >= nametbl->cap)
	{
		nametbl->cap = 2 * (nametbl->size + 1);
		nametbl->cell = (cell_t *)reallocarray(nametbl->cell, nametbl->cap, sizeof(cell_t));
		if(nametbl->cell == NULL)
		{
			print_err_msg("nametable overflow");
			abort();
		}
	}

	nametbl->cell[nametbl->size].name = name;
	
	if(nametbl->size)
		nametbl->cell[nametbl->size].id = nametbl->cell[nametbl->size - 1].id + 1;
	else
		nametbl->cell[0].id = 0; /* initial id */

	return nametbl->cell[nametbl->size++].id;
}

static size_t TblGetID(const char *name, nametbl_t *nametbl)
{
	assert(name);
	assert(nametbl);

	for (size_t i = 0; i < nametbl->size; i++)
		if(strcmp(name, nametbl->cell[i].name) == 0)
			return nametbl->cell[i].id;

	return TblAddName(name, nametbl);
}

static void TblDestroy(nametbl_t *nametbl)
{
	if(nametbl == NULL)
		return;

	free(nametbl->cell);
	nametbl->cell = NULL;
	nametbl->size = nametbl->cap = 0;

	free(nametbl);
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

static node_t *GetOp(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL;
	node_t *new_node = NULL;

	if(IS_(OPN_BRC, **data))
	{
		(*data)++;
		node = NewNode(OP_SEQ);

		new_node = GetOp(data, nametbl);
		if(new_node == NULL)	/* in general, it isn't necessary... */
			write_err("excepted operation(s)", (**data).line);

		while(new_node)
		{
			AddChild(node, new_node);
			new_node = GetOp(data, nametbl);
		}
		
		if(IS_(CLS_BRC, **data))
			(*data)++;
		else
			write_err("missing '}'", (**data).line);
	}
	else if((node = GetWhileIf(data, nametbl, IF))	||
			(node = GetWhileIf(data, nametbl, WHILE)));
	else if((node = GetAsm(data))				||
			(node = GetReturn(data, nametbl))	||
			(node = GetCallFunc(data, nametbl))	||
			(node = GetAssign(data, nametbl))	||
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
	nametbl_t *nametbl = NULL;

	if (IS_(FUNC, **data))
	{
		(*data)++;
		if((**data).type == TP_IDENT && IS_(OPN_PAR, (*data)[1]))
		{
			node = NewNode(FUNC_DECL((**data).val.name));
			(*data) += 2;
			AddChild(node, NewNode(PARAM));
			
			nametbl = TblInit();
			
			while((**data).type == TP_IDENT)	/* arguments */
			{
				arg_node = GetVar(data, nametbl);
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
			
			node->child->node->data.val.id = nametbl->size;

			if((arg_node = GetOp(data, nametbl)))
				AddChild(node, arg_node);
			else
				write_err("excepted function's body", (**data).line);
		}
		else
			write_err("function's signature missing", (**data).line);
	}

	TblDestroy(nametbl);
	return node;
}

static node_t *GetCallFunc(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL, *arg_node = NULL;

	if((**data).type == TP_IDENT && IS_(OPN_PAR, (*data)[1]))
	{
		node = NewNode(FUNC_CALL((**data).val.name));
		AddChild(node, NewNode(PARAM));
		(*data) += 2;

		size_t param_count = 0;
		while ((arg_node = GetOrExpr(data, nametbl)))
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

static node_t *GetReturn(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL, *arg_node = NULL;

	if(IS_(RETURN, **data))
	{
		node = NewNode(RETURN);
		(*data)++;

		if(!IS_(SEMICOLON, **data))
		{
			arg_node = GetOrExpr(data, nametbl);
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

static node_t *GetWhileIf(node_data_t *data[], nametbl_t *nametbl, const node_data_t while_or_if)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL;
	node_t *new_node = NULL;

	if(IS_(while_or_if, **data))
	{
		(*data)++;
		
		node = NewNode(while_or_if);
		
		if(IS_(OPN_PAR, **data))
		{
			(*data)++;
			
			if((new_node = GetOrExpr(data, nametbl)))
			{
				AddChild(node, new_node);
				
				if(IS_(CLS_PAR, **data))
					(*data)++;
				else
					write_err("missing ')'", (**data).line);

				if((new_node = GetOp(data, nametbl)))
					AddChild(node, new_node);
				else
					write_err("excepted operations body", (**data).line);
				
				if(IS_(ELSE, **data))
				{
					(*data)++;
					
					if((new_node = GetOp(data, nametbl)))
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

static node_t *GetAsm(node_data_t *data[])
{
	assert(data);
	assert(*data);

	node_t *node = NULL;
	if (IS_(ASM, **data))
	{
		(*data)++;
		
		if(IS_(OPN_PAR, **data))
		{
			(*data)++;
			
			if((**data).type == TP_LITERAL)
			{
				node = NewNode(ASM);
				AddChild(node, NewNode(**data));
				(*data)++;
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

static node_t *GetAssign(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL;
	node_t *new_node = NULL;

	if((new_node = GetVarExpr(data, nametbl)))
	{
		if(IS_(ASSIGN, **data))
		{
			(*data)++;

			node = NewNode(ASSIGN);
			AddChild(node, new_node);

			if((new_node = GetOrExpr(data, nametbl)))
				AddChild(node, new_node);
			else
				write_err("missing rvalue", (**data).line);
		}
		else
			TreeDestroy(new_node);
	}

	return node;
}

static node_t *GetOrExpr(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = GetAndExpr(data, nametbl), *new_node = NULL, *arg_node = NULL;
	if(node == NULL)
		return NULL;
	
	while(IS_(OR, **data))
	{
		(*data)++;

		if((arg_node = GetAndExpr(data, nametbl)))
		{
			new_node = NewBinNode(OR, node, arg_node);
			node = new_node;
		}
		else
			write_err("missing expression (right from 'or')", (**data).line);
	}

	return node;
}

static node_t *GetAndExpr(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = GetCompExpr(data, nametbl), *new_node = NULL, *arg_node = NULL;
	if(node == NULL)
		return NULL;
	
	while(IS_(AND, **data))
	{
		(*data)++;

		if((arg_node = GetCompExpr(data, nametbl)))
		{
			new_node = NewBinNode(AND, node, arg_node);
			node = new_node;
		}
		else
			write_err("missing expression (right from 'and')", (**data).line);
	}

	return node;
}

static node_t *GetCompExpr(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = GetExpr(data, nametbl), *arg_node = NULL;
	if(node == NULL)
		return NULL;

	if(IS_(GREATER, **data) || IS_(LESS, **data) || IS_(EQ, **data))
	{
		node_data_t op = **data;
		(*data)++;

		if((arg_node = GetExpr(data, nametbl)))
			node = NewBinNode(op, node, arg_node);
		else
			write_err("missing expression (right from compare)", (**data).line);
	}

	return node;
}

static node_t *GetExpr(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *new_node = NULL, *arg_node = NULL;
	node_t *node = GetTemp(data, nametbl);
	
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
		
		if((arg_node = GetTemp(data, nametbl)))
			AddChild(new_node, arg_node);
		else
			write_err("missing right expression (from '+' or '-')", (**data).line);

		node = new_node;
	}

	return node;
}

static node_t *GetTemp(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *new_node = NULL, *arg_node = NULL;
	node_t *node = GetPrim(data, nametbl);
	if(node == NULL)
		return NULL;

	while(IS_(MUL, **data) || IS_(DIV, **data))
	{
		new_node = NewNode(**data);
		(*data)++;

		AddChild(new_node, node);
		
		if((arg_node = GetPrim(data, nametbl)))
			AddChild(new_node, arg_node);
		else
			write_err("missing right expression (from '*' or '/')", (**data).line);

		node = new_node;
	}

	return node;
}

static node_t *GetPrim(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL;

	if(IS_(OPN_PAR, **data))
	{
		(*data)++;
		if((node = GetOrExpr(data, nametbl)))
		{
			if(IS_(CLS_PAR, **data))
				(*data)++;
			else
				write_err("missing ')'", (**data).line);
		}
		else
			write_err("missing expression", (**data).line);
	}
	else if((node = GetCallFunc(data, nametbl)));
	else if((node = GetNum(data)));
	else
		node = GetVarExpr(data, nametbl);

	return node;
}


static node_t *GetVarExpr(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL;
	
	if((node = GetVarAdr(data, nametbl)));
	else if((node = GetDeref(data, nametbl)));
	else
		node = GetVar(data, nametbl);

	return node;
}

static node_t *GetVarAdr(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL;

	if((**data).type == TP_TAKEADDR)
	{
		(*data)++;

		if ((**data).type == TP_IDENT)
		{
			node = NewNode(TAKEADDR(TblGetID((**data).val.name, nametbl)));

			(*data)++;
		}
		else
			write_err("excepted variable after '&'", (**data).line);
	}

	return node;
}

static node_t *GetDeref(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	assert(nametbl);

	node_t *node = NULL, *op_node = NULL;
	
	if(IS_(OPN_BRK, **data))
	{
		(*data)++;

		if((op_node = GetOrExpr(data, nametbl)))
		{
			node = NewNode(DEREF);
			AddChild(node, op_node);
		}
		else
			write_err("excepted expression in '[]'", (**data).line);
		
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

static node_t *GetVar(node_data_t *data[], nametbl_t *nametbl)
{
	assert(data);
	assert(*data);
	
	if((**data).type == TP_IDENT)
	{
		node_t *node = NewNode(VAR(TblGetID((**data).val.name, nametbl)));
		
		(*data)++;
		return node;
	}

	return NULL;
}
/*-------------------------------------------*/
#include "undef_macro.h"
