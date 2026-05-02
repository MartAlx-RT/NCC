#include "ncc.h"

static int COMPILE_STATUS = 0;	/* 0 - normal, 1 - error */

static size_t LBL_CNT = 0;		/* global label counter */
static size_t LOOP_LBL_CNT = 0;

static FILE *ASM_OUT = NULL;	/* global pointer to asm file */

/*---------------------------------------------*/
static void GnrtDeclFunc(const node_t *ast);
static void GnrtAsm(const node_t *ast);
static void GnrtArifm(const node_t *ast);

static void GnrtExpr(const node_t *ast);
static void GnrtOr(const node_t *ast);
static void GnrtAnd(const node_t *ast);
static void GnrtComp(const node_t *ast, const op_t gle); /* Greater Less Equal*/

static void GnrtIf(const node_t *ast);
static void GnrtOpSeq(const node_t *ast);
static void GnrtOp(const node_t *ast);
static void GnrtAssign(const node_t *ast);
static void GnrtWhile(const node_t *ast);
static void GnrtCallFunc(const node_t *ast);

static void GnrtReturn(const node_t *ast);
static void GnrtBreak(const node_t *ast);
static void GnrtContinue(const node_t *ast);

static void GnrtDeref(const node_t *ast);
/*---------------------------------------------*/
#include "def_macro.h"
/*---------------------------------------------*/
int CompileTree(const node_t *ast, FILE *asm_out)
{
	if(ast == NULL || asm_out == NULL)
	{
		print_err_msg("nullptr passed as arg(s)");
		return 1;
	}
	if(!IS_(ROOT, ast->data))
	{
		print_err_msg("I wanna be a root");
		return 1;
	}

	ASM_OUT = asm_out;

	child_t *decl = ast->child;
	while(decl)
	{
		if(decl->node == NULL)
		{
			print_err_msg("null node");
			return 1;
		}
		if(COMPILE_STATUS)
		{
			print_err_msg("compilation failed");
			return 1;
		}

		GnrtDeclFunc(decl->node);
		decl = decl->next;
	}

	return COMPILE_STATUS;
}

static void GnrtOp(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;

	switch(ast->data.type)
	{
	case TP_CALL_FUNC:
		GnrtCallFunc(ast);
		return;
	case TP_KWORD:
		switch(ast->data.val.kword)
		{
		case KW_ASM:
			GnrtAsm(ast);
			return;
		case KW_IF:
			GnrtIf(ast);
			return;
		case KW_WHILE:
			GnrtWhile(ast);
			return;
		case KW_RETURN:
			GnrtReturn(ast);
			return;
		case KW_PASS:
			return;
		case KW_BREAK:
			GnrtBreak(ast);
			return;
		case KW_CONTINUE:
			GnrtContinue(ast);
			return;
		case KW_ELSE:
		case KW_FOR:		/* aren't implemented */
		case KW_FUNC:
		default:
			err_exit_msg("invalid keyword");
		}
		return;
	case TP_OP:
		switch(ast->data.val.op)
		{
		case OP_ASSIGN:
			GnrtAssign(ast);
			return;
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_GREATER:		/* implemented in GnrtArifm */
		case OP_LESS:
		case OP_EQ:
		case OP_OR:
		case OP_AND:
			GnrtArifm(ast);
			return;
		default:
			err_exit_msg("invalid operation");
		}
		return;
	case TP_EOF:
	case TP_ROOT:
	case TP_NUM:
	case TP_OP_SEQ:
	case TP_IDENT:
	case TP_PARAM:				/* cannot be operation */
	case TP_VAR:
	case TP_SYMB:
	case TP_DECL_FUNC:
	case TP_LITERAL:
	case TP_DEREF:
	case TP_TAKEADDR:
	default:
		err_exit_msg("invalid type");
	}
}

static void GnrtOpSeq(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;		
	
	if (ast->data.type != TP_OP_SEQ)
		return GnrtOp(ast);

	child_t *op = ast->child;
	while(op)
	{
		if(op->node == NULL)
			err_exit_msg("child->node == NULL");
		GnrtOp(op->node);
		op = op->next;
	}
}

static void GnrtArifm(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;

	switch(ast->data.type)
	{
	case TP_NUM:
		print_asm("\tpush\t%ld\t; num\n", ast->data.val.num);
		return;
	case TP_VAR:
//		print_asm("\tmov\trbp, rsp\n"
//			  "add rbp, %lu ;calculate var pos in stack\n"
//			  "push [rbp] ;push var\n\n",
//				  ast->data.val.id);
		print_asm("\tpush\tqword [rbp%+ld*8]\t; var\n", ast->data.val.id);
		return;
	case TP_DEREF:
		GnrtDeref(ast);
		return;
	case TP_TAKEADDR:
		print_asm
			(
			 "that isn't implemented!!! mov\trbp, %lu\n"
			 "add\trbp, rsp\n"
			 "push\trbp ;&\n\n",
			 ast->data.val.id
			);
		return;
	case TP_OP:
		if(!IS_BINNODE(ast))
			err_exit_msg("invalid node");

		GnrtExpr(LEFT(ast));
		GnrtExpr(RIGHT(ast));

		print_asm
			(
			 "\tpop\trax\n"
			 "\tpop\trdx\n"
			);
		switch(ast->data.val.op)
		{
			case OP_ADD:
				print_asm("\tadd\t");
				break;
			case OP_SUB:
				print_asm("\tsub\t");
				break;
			case OP_MUL:
				print_asm("\timul\t");
				break;
			case OP_DIV:
				print_asm("\tthat isn't implemented!!! div\n");
				break;
			case OP_GREATER:
			case OP_LESS:
			case OP_ASSIGN:
			case OP_EQ:			/* implemented in GnrtComp, GnrtAnd, GnrtOr */
			case OP_OR:
			case OP_AND:
			default:
				err_exit_msg("invalid operation");
		}
		print_asm("rax, rdx\t; arifm\n");
		return;
	case TP_EOF:
	case TP_ROOT:
	case TP_OP_SEQ:
	case TP_IDENT:
	case TP_PARAM:
	case TP_KWORD:
	case TP_SYMB:
	case TP_DECL_FUNC:
	case TP_CALL_FUNC:
	case TP_LITERAL:
	default:
		err_exit_msg("invalid type");
	}
}

static void GnrtDeclFunc(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(ast->data.type != TP_DECL_FUNC)
		err_exit_msg("node is not a function declaration");
	if(!IS_BINNODE(ast))
		err_exit_msg("invalid declaration");

	print_asm
		(
		 "\n%s:\t; <decl>\n"
		 "\tenter\t%ld*8, 0\n",
		 ast->data.val.name,
		 ast->child->node->data.val.id + ast->child->next->node->data.val.id
		);

	GnrtOpSeq(RIGHT(ast));
	
	//print_asm("\tret\n\n"); /* dubiously */
}

static void GnrtAsm(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;
	if(!IS_(ASM, ast->data))
		err_exit_msg("is not 'asm'");
	if(!(ast->child && ast->child->node))
		err_exit_msg("invalid node");

	print_asm
		(
		 "; <asm inline>:\n"
		 "%s\n",
		 ast->child->node->data.val.name
		);
}

static void GnrtOr(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(OR, ast->data))
		err_exit_msg("is not 'or'");
	if(!IS_BINNODE(ast))
		err_exit_msg("not binary 'or'");

	GnrtExpr(RIGHT(ast));
	GnrtExpr(LEFT(ast));

	print_asm("\tins't implemented!!! ;or\n"
			  "pop rax ;lvalue\n"
			  "pop rdx ;rvalue\n"
			  "cmp rax, 0\n"
			  "jne .L%lu\n"
			  "cmp rdx, 0\n"
			  "jne .L%lu\n"
			  ";result:\n"
			  "push 0\n"
			  "jmp .L%lu\n"
			  ".L%lu:\n"
			  "push 1\n"
			  ".L%lu:\n\n",
			  LBL_CNT,
			  LBL_CNT,
			  LBL_CNT + 1,
			  LBL_CNT,
			  LBL_CNT + 1);
	LBL_CNT += 2;
}

static void GnrtAnd(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(AND, ast->data))
		err_exit_msg("is not 'and'");
	if(!IS_BINNODE(ast))
		err_exit_msg("not binary 'and'");

	GnrtExpr(RIGHT(ast));
	GnrtExpr(LEFT(ast));

	print_asm("\tins't implemented!!! ;and\n"
			  "pop rax ;lvalue\n"
			  "pop rdx ;rvalue\n"
			  "cmp rax, 0\n"
			  "je .L%lu\n"
			  "cmp rdx, 0\n"
			  "je .L%lu\n"
			  ";result:\n"
			  "push 1\n"
			  "jmp .L%lu\n"
			  ".L%lu:\n"
			  "push 0\n"
			  ".L%lu:\n\n",
			  LBL_CNT,
			  LBL_CNT,
			  LBL_CNT + 1,
			  LBL_CNT,
			  LBL_CNT + 1);
	LBL_CNT += 2;
}

static void GnrtComp(const node_t *ast, const op_t gle)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	
	if(!IS_BINNODE(ast))
		err_exit_msg("node is not binary");
	
	GnrtExpr(RIGHT(ast));
	GnrtExpr(LEFT(ast));

	const char *jmp_type = NULL;
	switch(gle)
	{
		case OP_GREATER:
			jmp_type = "ja";
			break;
		case OP_LESS:
			jmp_type = "jb";
			break;
		case OP_EQ:
			jmp_type = "je";
			break;
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_ASSIGN:
		case OP_OR:
		case OP_AND:
		default:
			err_exit_msg("jmptype out of range");
	}

	print_asm
		(
		 "\t; <cmp>:\n"
		 "\tpop\trax\t; lval\n"
		 "\tpop\trdx\t; rval\n"
		 "\tcmp\trdx, rax\n"
		 "\t%s\t.L%lu\n"
		 "\tpush\t0\n"
		 "\tjmp\t.L%lu\n"
		 ".L%lu:\n"
		 "\tpush\t1\n"
		 ".L%lu:\n",
		 jmp_type, LBL_CNT,
		 LBL_CNT + 1,
		 LBL_CNT,
		 LBL_CNT + 1
		);

	LBL_CNT += 2;
}

static void GnrtExpr(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
		
	switch(ast->data.type)
	{
		case TP_NUM:
		case TP_VAR:
		case TP_TAKEADDR:
			GnrtArifm(ast);
			break;
		case TP_CALL_FUNC:
			GnrtCallFunc(ast);
			break;
		case TP_DEREF:
			GnrtDeref(ast);
			break;
		case TP_OP:
			switch(ast->data.val.op)
			{
				case OP_ADD:
				case OP_SUB:
				case OP_MUL:
				case OP_DIV:
					GnrtArifm(ast);
					break;
				case OP_AND:
					GnrtAnd(ast);
					break;
				case OP_OR:
					GnrtOr(ast);
					break;
				case OP_GREATER:
				case OP_LESS:
				case OP_EQ:
					GnrtComp(ast, ast->data.val.op);
					break;
				case OP_ASSIGN:
				default:
					err_exit_msg("invalid operation");
			}
			break;
		case TP_EOF:
		case TP_ROOT:
		case TP_OP_SEQ:
		case TP_IDENT:
		case TP_PARAM:			/* cannot be expression */
		case TP_KWORD:
		case TP_SYMB:
		case TP_DECL_FUNC:
		case TP_LITERAL:
		default:
			err_exit_msg("type out of range");
	}
}

static void GnrtIf(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(IF, ast->data))
		err_exit_msg("is not 'if'");

	child_t *child = ast->child;
	if(CHILD_EXISTS(child))
		GnrtExpr(child->node);
	else
		err_exit_msg("missing condition");
	child = child->next;

	size_t else_lbl = LBL_CNT++, endif_lbl = LBL_CNT++;
	
	/* condition */
	print_asm
		(
		 "\t; <if cnd>:\n"
		 "\tpop\trax\n"
		 "\ttest\trax, rax\n"
		 "\tje\t.L%lu\n"
		 "\t; <if body>\n",
		 else_lbl
		);

	/* 'if' body */
	if(CHILD_EXISTS(child))
		GnrtOpSeq(child->node);
	else
		err_exit_msg("missing 'if' body");
	child = child->next;

	/* 'else' body */
	print_asm
		(
		 "\tjmp\t.L%lu\n"
		 ".L%lu:\n"
		 "\t; <else body>\n",
		 endif_lbl, else_lbl
		);

	if(CHILD_EXISTS(child))
		GnrtOpSeq(child->node);

	/* endif */
	print_asm("\t.L%lu:\t; <endif>\n", endif_lbl);
}

static void GnrtWhile(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(WHILE, ast->data))
		err_exit_msg("is not 'while'");
	if(!IS_BINNODE(ast))
		err_exit_msg("is not binary");

	size_t cond_lbl = LOOP_LBL_CNT++, end_lbl = LOOP_LBL_CNT++;
	print_asm("\t;while condition\n"
			  ".Lloop%lu:\n",
			  cond_lbl);

	GnrtExpr(LEFT(ast));

	print_asm("\t;check condition\n"
			  "pop rax\n"
			  "cmp rax, 0\n"
			  "je .Lloop%lu\n"
			  ";while body\n",
			  end_lbl);

	GnrtOpSeq(RIGHT(ast));

	print_asm("\tjmp .Lloop%lu\n"
			  ";end while\n"
			  ".Lloop%lu:\n\n",
			  cond_lbl, end_lbl);
}

static void GnrtAssign(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(ASSIGN, ast->data))
		err_exit_msg("is not 'assignment'");
	if(!IS_BINNODE(ast))
		err_exit_msg("is not binary");

	GnrtExpr(RIGHT(ast));

	if(LEFT(ast)->data.type == TP_VAR)
	{
//		print_asm("\tmov rbp, rsp\n"
//				  "add rbp, %lu ;calculate var pos in stack\n"
//				  "pop [rbp] ;assign var a value\n\n",
//				  LEFT(ast)->data.val.id);

		print_asm
			(
			 "\tpop\tqword [rbp%+ld*8]\t; assign\n",
			 LEFT(ast)->data.val.id
			);
	}
	else if(LEFT(ast)->data.type == TP_DEREF)
	{
		if(!CHILD_EXISTS(LEFT(ast)->child))
			err_exit_msg("wrong node");

		GnrtExpr(LEFT(ast)->child->node);
		print_asm("\tisn't implemented!!! pop rbx\n"
				  "pop [rbx] ;assign [] a value\n\n");
	}
	else
		err_exit_msg("lvalue must be variable or dereference ptr");
}

static void GnrtCallFunc(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(ast->data.type != TP_CALL_FUNC)
		err_exit_msg("is not a function call");
	if(ast->child == NULL || ast->child->node == NULL)
		err_exit_msg("call node without parameters node");

	print_asm("\t; <call>:\n");

	node_t *param_node = ast->child->node;

	if(param_node->child)
	{
		child_t *param = param_node->child->prev;	// param = last_func_arg

		do
		{
			print_asm("\tpush\tqword [rbp%+ld]\t; ld func arg\n", param->node->data.val.id);
			param = param->prev;
		}
		while(param != param_node->child);
	}

	print_asm("\tcall\t%s\n", ast->data.val.name);

//	while (param)
//	{
//		if(param->node == NULL)
//			err_exit_msg("non-existing param");
//
//		GnrtExpr(param->node);
//
//		param = param->next;
//	}
//	
//	/* load parameters to ram stack */
//	print_asm("\tmov rbp, rsp\n"
//			  "add rbp, %lu ;last param pos\n",
//			  n_var + ast->child->node->data.val.id - 1);
//
//							/* num of parameters */
//	for (size_t i = 0; i < ast->child->node->data.val.id; i++) 
//	{
//		print_asm("\tpop [rbp] ;load param\n"
//				  "sub rbp, 1\n");
//	}
//
//	print_asm("\tadd rsp, %lu ;shift sp\n"
//			  "call %s\n"
//			  "sub rsp, %lu ;shift back\n"
//			  ";-------------\n\n",
//			  n_var, ast->data.val.name, n_var);
}

static void GnrtReturn(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(RETURN, ast->data))
		err_exit_msg("is not a 'return'");
	
	if(ast->child && ast->child->node)
		GnrtExpr(ast->child->node);
	
	print_asm
		(
		 "\tleave\n"
		 "\tret\n"
		);
}

static void GnrtBreak(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(BREAK, ast->data))
		err_exit_msg("is not 'break'");

	print_asm("\tjmp .Lloop%lu\t;break\n", LOOP_LBL_CNT - 1);
}

static void GnrtContinue(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(CONTINUE, ast->data))
		err_exit_msg("is not 'continue'");

	print_asm("\tjmp .Lloop%lu\t;continue\n", LOOP_LBL_CNT - 2);
}

static void GnrtDeref(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(ast->data.type != TP_DEREF)
		err_exit_msg("is not '&'");
	if(!CHILD_EXISTS(ast->child))
		err_exit_msg("wrong node");

	GnrtExpr(ast->child->node);

	print_asm("\tpop rbp\n"
			  "push [rbp] ;push deref ptr\n\n");
}
/*---------------------------------------------*/
#include "undef_macro.h"
/*---------------------------------------------*/
