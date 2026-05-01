#include "ncc.h"

static int COMPILE_STATUS = 0;	/* 0 - normal, 1 - error */

static size_t LBL_CNT = 0;		/* global label counter */
static size_t LOOP_LBL_CNT = 0;

static FILE *ASM_OUT = NULL;	/* global pointer to asm file */

/*---------------------------------------------*/
static void GnrtDeclFunc(const node_t *ast);
static void GnrtAsm(const node_t *ast);
static void GnrtArifm(const node_t *ast, const size_t n_var);

static void GnrtExpr(const node_t *ast, const size_t n_var);
static void GnrtOr(const node_t *ast, const size_t n_var);
static void GnrtAnd(const node_t *ast, const size_t n_var);
static void GnrtComp(const node_t *ast, const size_t n_var, const op_t gle); /* Greater Less Equal*/

static void GnrtIf(const node_t *ast, const size_t n_var);
static void GnrtOpSeq(const node_t *ast, const size_t n_var);
static void GnrtOp(const node_t *ast, const size_t n_var);
static void GnrtAssign(const node_t *ast, const size_t n_var);
static void GnrtWhile(const node_t *ast, const size_t n_var);
static void GnrtCallFunc(const node_t *ast, const size_t n_var);

static void GnrtReturn(const node_t *ast, const size_t n_var);
static void GnrtBreak(const node_t *ast);
static void GnrtContinue(const node_t *ast);

static void GnrtDeref(const node_t *ast, const size_t n_var);

static size_t GetMaxID(const node_t *ast);
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

static void GnrtOp(const node_t *ast, const size_t n_var)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;

	switch(ast->data.type)
	{
	case TP_CALL_FUNC:
		GnrtCallFunc(ast, n_var);
		return;
	case TP_KWORD:
		switch(ast->data.val.kword)
		{
		case KW_ASM:
			GnrtAsm(ast);
			return;
		case KW_IF:
			GnrtIf(ast, n_var);
			return;
		case KW_WHILE:
			GnrtWhile(ast, n_var);
			return;
		case KW_RETURN:
			GnrtReturn(ast, n_var);
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
			GnrtAssign(ast, n_var);
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
			GnrtArifm(ast, n_var);
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

static void GnrtOpSeq(const node_t *ast, const size_t n_var)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;		
	
	if (ast->data.type != TP_OP_SEQ)
		return GnrtOp(ast, n_var);

	child_t *op = ast->child;
	while(op)
	{
		if(op->node == NULL)
			err_exit_msg("child->node == NULL");
		GnrtOp(op->node, n_var);
		op = op->next;
	}
}

static void GnrtArifm(const node_t *ast, const size_t n_var)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;

	switch(ast->data.type)
	{
	case TP_NUM:
		print_asm("push\t%ld\t; num\n", ast->data.val.num);
		return;
	case TP_VAR:
//		print_asm("mov\trbp, rsp\n"
//			  "add rbp, %lu ;calculate var pos in stack\n"
//			  "push [rbp] ;push var\n\n",
//				  ast->data.val.id);
		print_asm("push\tqword [rbp%+ld*8]\t; var\n", ast->data.val.id);
		return;
	case TP_DEREF:
		GnrtDeref(ast, n_var);
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

		GnrtExpr(LEFT(ast), n_var);
		GnrtExpr(RIGHT(ast), n_var);

		print_asm
			(
			 "pop\trax\n"
			 "pop\trdx\n"
			);
		switch(ast->data.val.op)
		{
			case OP_ADD:
				print_asm("add\t");
				return;
			case OP_SUB:
				print_asm("sub\t");
				return;
			case OP_MUL:
				print_asm("imul\t");
				return;
			case OP_DIV:
				print_asm("that isn't implemented!!! div\n");
				return;
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
		err_exit_msg("invalid node");

	print_asm
		(
		 "%s:\t; decl\n"
		 "push\trbp\n"
		 "mov\trbp, rsp\n",
		 ast->data.val.name
		);

	GnrtOpSeq(RIGHT(ast), GetMaxID(ast));
	
	//print_asm("ret\n\n"); /* dubiously */
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

static void GnrtOr(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(OR, ast->data))
		err_exit_msg("is not 'or'");
	if(!IS_BINNODE(ast))
		err_exit_msg("not binary 'or'");

	GnrtExpr(RIGHT(ast), n_var);
	GnrtExpr(LEFT(ast), n_var);

	print_asm("ins't implemented!!! ;or\n"
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

static void GnrtAnd(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(AND, ast->data))
		err_exit_msg("is not 'and'");
	if(!IS_BINNODE(ast))
		err_exit_msg("not binary 'and'");

	GnrtExpr(RIGHT(ast), n_var);
	GnrtExpr(LEFT(ast), n_var);

	print_asm("ins't implemented!!! ;and\n"
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

static void GnrtComp(const node_t *ast, const size_t n_var, const op_t gle)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	
	if(!IS_BINNODE(ast))
		err_exit_msg("node is not binary");
	
	GnrtExpr(RIGHT(ast), n_var);
	GnrtExpr(LEFT(ast), n_var);

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
		 "; <cmp>:\n"
		 "pop\trax\t; lval\n"
		 "pop\trdx\t; rval\n"
		 "cmp\trdx, rax\n"
		 "%s\t.L%lu\n"
		 "push\t0\n"
		 "jmp\t.L%lu\n"
		 ".L%lu:\n"
		 "push\t1\n"
		 ".L%lu:\n",
		 jmp_type, LBL_CNT,
		 LBL_CNT + 1,
		 LBL_CNT,
		 LBL_CNT + 1
		);

	LBL_CNT += 2;
}

static void GnrtExpr(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
		
	switch(ast->data.type)
	{
	case TP_NUM:
	case TP_VAR:
	case TP_TAKEADDR:
		GnrtArifm(ast, n_var);
		break;
	case TP_CALL_FUNC:
		GnrtCallFunc(ast, n_var);
		break;
	case TP_DEREF:
		GnrtDeref(ast, n_var);
		break;
	case TP_OP:
		switch(ast->data.val.op)
		{
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
			GnrtArifm(ast, n_var);
			break;
		case OP_AND:
			GnrtAnd(ast, n_var);
			break;
		case OP_OR:
			GnrtOr(ast, n_var);
			break;
		case OP_EQ:
			GnrtComp(ast, n_var, OP_EQ);
			break;
		case OP_GREATER:
			GnrtComp(ast, n_var, OP_GREATER);
			break;
		case OP_LESS:
			GnrtComp(ast, n_var, OP_LESS);
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

static void GnrtIf(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(IF, ast->data))
		err_exit_msg("is not 'if'");

	child_t *child = ast->child;
	if(CHILD_EXISTS(child))
		GnrtExpr(child->node, n_var);
	else
		err_exit_msg("missing condition");
	child = child->next;

	size_t else_lbl = LBL_CNT++, endif_lbl = LBL_CNT++;
	
	/* condition */
	print_asm
		(
		 "; <if cnd>:\n"
		 "pop\trax\n"
		 "test\trax, rax\n"
		 "je\t.L%lu\n"
		 "; <if body>\n",
		 else_lbl
		);

	/* 'if' body */
	if(CHILD_EXISTS(child))
		GnrtOpSeq(child->node, n_var);
	else
		err_exit_msg("missing 'if' body");
	child = child->next;

	/* 'else' body */
	print_asm
		(
		 "jmp\t.L%lu\n"
		 ".L%lu:\n"
		 "; <else body>\n",
		 endif_lbl, else_lbl
		);

	if(CHILD_EXISTS(child))
		GnrtOpSeq(child->node, n_var);

	/* endif */
	print_asm(".L%lu:\t; <endif>\n", endif_lbl);
}

static void GnrtWhile(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(WHILE, ast->data))
		err_exit_msg("is not 'while'");
	if(!IS_BINNODE(ast))
		err_exit_msg("is not binary");

	size_t cond_lbl = LOOP_LBL_CNT++, end_lbl = LOOP_LBL_CNT++;
	print_asm(";while condition\n"
			  ".Lloop%lu:\n",
			  cond_lbl);

	GnrtExpr(LEFT(ast), n_var);

	print_asm(";check condition\n"
			  "pop rax\n"
			  "cmp rax, 0\n"
			  "je .Lloop%lu\n"
			  ";while body\n",
			  end_lbl);

	GnrtOpSeq(RIGHT(ast), n_var);

	print_asm("jmp .Lloop%lu\n"
			  ";end while\n"
			  ".Lloop%lu:\n\n",
			  cond_lbl, end_lbl);
}

static void GnrtAssign(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(ASSIGN, ast->data))
		err_exit_msg("is not 'assignment'");
	if(!IS_BINNODE(ast))
		err_exit_msg("is not binary");

	GnrtExpr(RIGHT(ast), n_var);

	if(LEFT(ast)->data.type == TP_VAR)
	{
//		print_asm("mov rbp, rsp\n"
//				  "add rbp, %lu ;calculate var pos in stack\n"
//				  "pop [rbp] ;assign var a value\n\n",
//				  LEFT(ast)->data.val.id);

		print_asm
			(
			 "pop\tqword [rbp%+ld]\t; assign\n",
			 LEFT(ast)->data.val.id
			);
	}
	else if(LEFT(ast)->data.type == TP_DEREF)
	{
		if(!CHILD_EXISTS(LEFT(ast)->child))
			err_exit_msg("wrong node");

		GnrtExpr(LEFT(ast)->child->node, n_var);
		print_asm("isn't implemented!!! pop rbx\n"
				  "pop [rbx] ;assign [] a value\n\n");
	}
	else
		err_exit_msg("lvalue must be variable or dereference ptr");
}

static void GnrtCallFunc(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(ast->data.type != TP_CALL_FUNC)
		err_exit_msg("is not a function call");
	if(ast->child == NULL || ast->child->node == NULL)
		err_exit_msg("call node without parameters node");

	print_asm("isn't implemented!!! ;call function\n"
			  ";-------------\n");
	// print_asm(";call function\n"
	// 		  "mov rdi, rsp\n"
	// 		  "add rdi, %lu ;di points to new memory segment\n",
	// 		  n_var);

	child_t *param = ast->child->node->child;
	/* push to arifmetic stack */
	while (param)
	{
		if(param->node == NULL)
			err_exit_msg("non-existing param");

		GnrtExpr(param->node, n_var);

		param = param->next;
	}
	
	/* load parameters to ram stack */
	print_asm("mov rbp, rsp\n"
			  "add rbp, %lu ;last param pos\n",
			  n_var + ast->child->node->data.val.id - 1);

							/* num of parameters */
	for (size_t i = 0; i < ast->child->node->data.val.id; i++) 
	{
		print_asm("pop [rbp] ;load param\n"
				  "sub rbp, 1\n");
	}

	print_asm("add rsp, %lu ;shift sp\n"
			  "call %s\n"
			  "sub rsp, %lu ;shift back\n"
			  ";-------------\n\n",
			  n_var, ast->data.val.name, n_var);
}

static void GnrtReturn(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(RETURN, ast->data))
		err_exit_msg("is not a 'return'");
	
	if(ast->child && ast->child->node)
		GnrtExpr(ast->child->node, n_var);
	
	print_asm
		(
		 "leave\n"
		 "ret\n"
		);
}

static void GnrtBreak(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(BREAK, ast->data))
		err_exit_msg("is not 'break'");

	print_asm("jmp .Lloop%lu\t;break\n", LOOP_LBL_CNT - 1);
}

static void GnrtContinue(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(CONTINUE, ast->data))
		err_exit_msg("is not 'continue'");

	print_asm("jmp .Lloop%lu\t;continue\n", LOOP_LBL_CNT - 2);
}

static void GnrtDeref(const node_t *ast, const size_t n_var)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(ast->data.type != TP_DEREF)
		err_exit_msg("is not '&'");
	if(!CHILD_EXISTS(ast->child))
		err_exit_msg("wrong node");

	GnrtExpr(ast->child->node, n_var);

	print_asm("pop rbp\n"
			  "push [rbp] ;push deref ptr\n\n");
}

/* counts number of variables in ast */
static void MaxID(const node_t *ast, size_t *max_id)
{
	assert(max_id);
	if (ast == NULL)
		return;
	
	if(ast->data.type == TP_VAR && ast->data.val.id > *max_id)
		*max_id = ast->data.val.id;

	if (ast->child)
	{
		child_t *child = ast->child;
		while (child)
		{
			MaxID(child->node, max_id);
			child = child->next;
		}
	}
}

static size_t GetMaxID(const node_t *ast)
{
	assert(ast);

	size_t max_id = 0;
	MaxID(ast, &max_id);

	return max_id + 1;
}
/*---------------------------------------------*/
#include "undef_macro.h"
/*---------------------------------------------*/
