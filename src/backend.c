#include "ncc.h"
#include "def_perror.h"
#include "def_grammar.h"
#include "def_emitters.h"
#include "emitter.h"
#include <fcntl.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <x86intrin.h>
#include <unistd.h>
#include <sys/wait.h>

typedef enum global_type_t
{
	GLOB_CONST,
	GLOB_STR,
} global_type_t;

typedef struct global_t
{
	global_type_t type;
	node_val_t val;
	ssize_t pos;
} global_t;

typedef struct globals_t
{
	global_t *globals;
	size_t size;
	size_t cap;
} globals_t;

static globals_t GLOBALS = (globals_t){ .globals = NULL, .size = 0, .cap = 0 };

static int COMPILE_STATUS = 0;	/* 0 - normal, 1 - error */

static size_t LBL_CNT = 0;	/* label counter */
static size_t LOOP_LBL_CNT = 0;

static FILE *ASM_OUT = NULL;	/* pointer to asm file */

/*---------------------------------------------*/
static size_t NewGlobal(const global_t global);

static void GenGlobals(void);

static void GenDeclFunc(const node_t *ast);
static void GenAsm(const node_t *ast);
static void GenArifm(const node_t *ast);

static void GenExpr(const node_t *ast);
static void GenOr(const node_t *ast);
static void GenAnd(const node_t *ast);
static void GenComp(const node_t *ast, const op_t gle); /* Greater Less Equal*/

static void GenIf(const node_t *ast);
static void GenOpSeq(const node_t *ast);
static void GenOp(const node_t *ast);
static void GenAssign(const node_t *ast);
static void GenWhile(const node_t *ast);
static void GenCallFunc(const node_t *ast);
static void GenStr(const node_t *ast);

static void GenReturn(const node_t *ast);
static void GenBreak(const node_t *ast);
static void GenContinue(const node_t *ast);

static void GenDeref(const node_t *ast);
/*---------------------------------------------*/
/*---------------------------------------------*/
int CompileTree(const node_t *ast, FILE *elf, FILE *nasm)
{
	if(ast == NULL || nasm == NULL)
	{
		print_err_msg("nullptr passed as arg(s)");
		return 1;
	}
	if(!IS_(ROOT, ast->data))
	{
		print_err_msg("I wanna be a root");
		return 1;
	}

	ASM_OUT = nasm;
	emitter_init(elf, nasm);

	CALL_L("main");
	MOV_RR(RDI, RAX);
	MOV_RI(RAX, 0x3c);
	SYSCALL;

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

		GenDeclFunc(decl->node);
		decl = decl->next;
	}

	GenGlobals();

	emitter_fixup();
	emitter_free_names();
	emitter_deinit();

	return COMPILE_STATUS;
}

static size_t NewGlobal(const global_t global)
{
	if(GLOBALS.size >= GLOBALS.cap)
	{
		GLOBALS.cap = (GLOBALS.cap+1)*2;
		GLOBALS.globals = (global_t *)reallocarray(GLOBALS.globals, GLOBALS.cap, sizeof(global_t));
		assert(GLOBALS.globals);
	}

	GLOBALS.globals[GLOBALS.size] = global;

	return GLOBALS.size++;
}

static void GenGlobals(void)
{
	fseek(emitter_get_elf(), (emitter_get_elf_pos()+7)/8 * 8, SEEK_SET);

	for(size_t i = 0; i < GLOBALS.size; i++)
	{
		if(GLOBALS.globals[i].type == GLOB_STR)
		{
			const char *str = GLOBALS.globals[i].val.name;

			const ssize_t current_pos = emitter_get_elf_pos();

			fseek(emitter_get_elf(), GLOBALS.globals[i].pos, SEEK_SET);
			MOV_RI(RAX, (ssize_t)(ELF_ENTRY_VA - ELF_START_OFF) + current_pos);
			fseek(emitter_get_elf(), current_pos, SEEK_SET);

			while(*str)	write_q((uint8_t)*str++);

			write_q(0);
		}
		else	err_exit_msg("global's type out of range");
	}
}
static void GenOp(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;

	switch(ast->data.type)
	{
		case TP_CALL_FUNC:	GenCallFunc(ast);	return;
		case TP_KWORD:
			switch(ast->data.val.kword)
			{
				case KW_ASM:		GenAsm(ast);		return;
				case KW_IF:		GenIf(ast);		return;
				case KW_WHILE:		GenWhile(ast);		return;
				case KW_RETURN:		GenReturn(ast);		return;
				case KW_PASS:					return;
				case KW_BREAK:		GenBreak(ast);		return;
				case KW_CONTINUE:	GenContinue(ast);	return;

				case KW_ELSE:
				case KW_FOR:		/* aren't implemented */
				case KW_FUNC:

				default:	err_exit_msg("invalid keyword");
			}
			return;

		case TP_OP:
			switch(ast->data.val.op)
			{
				case OP_ASSIGN:	GenAssign(ast);	return;

				case OP_ADD:
				case OP_SUB:
				case OP_MUL:
				case OP_DIV:
				case OP_MOD:
				case OP_GREATER:		/* implemented in GenArifm */
				case OP_LESS:
				case OP_EQ:
				case OP_NEQ:
				case OP_OR:
				case OP_AND:	GenArifm(ast);	return;

				default:	err_exit_msg("invalid operation");
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
		default:	err_exit_msg("invalid type");
	}
}

static void GenStr(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	assert(ast->data.type == TP_LITERAL);

	global_t global =
	{
		.pos = emitter_get_elf_pos(),
		.type = GLOB_STR,
		.val = ast->data.val
	};
	NewGlobal(global);

	MOV_RI(RAX, 0);		// not true mov, just a filler
	SHR_RI(RAX, 3);
	PUSH_R(RAX);
}

static void GenOpSeq(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;		
	
	if (ast->data.type != TP_OP_SEQ)
		return GenOp(ast);

	child_t *op = ast->child;
	while(op)
	{
		assert(op->node);
		GenOp(op->node);
		op = op->next;
	}
}

static void GenArifm(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;

	switch(ast->data.type)
	{
		case TP_NUM:
			PUSH_I((int32_t)ast->data.val.num);
			return;
		case TP_VAR:
			PUSH_M(RBP, 8*(int32_t)ast->data.val.id);
			return;
		case TP_DEREF:
			GenDeref(ast);
			return;
		case TP_TAKEADDR:
			LEA(RAX, RBP, 8*(int32_t)ast->data.val.id);
			SHR_RI(RAX, 3);
			PUSH_R(RAX);
			return;
		case TP_OP:
			if(!IS_BINNODE(ast))
				err_exit_msg("invalid node");

			GenExpr(LEFT(ast));
			GenExpr(RIGHT(ast));

			POP_R(RCX);
			POP_R(RAX);

			switch(ast->data.val.op)
			{
				case OP_ADD:
					ADD_RR(RAX, RCX);
					break;
				case OP_SUB:
					SUB_RR(RAX, RCX);
					break;
				case OP_MUL:
					IMUL_RR(RAX, RCX);
					break;
				case OP_DIV:
				case OP_MOD:
					MOV_RI(RDX, 0);
					IDIV_R(RCX);
					break;
				case OP_GREATER:
				case OP_LESS:
				case OP_ASSIGN:
				case OP_EQ:			/* implemented in GenComp, GenAnd, GenOr */
				case OP_NEQ:			/* implemented in GenComp, GenAnd, GenOr */
				case OP_OR:
				case OP_AND:

				default:	err_exit_msg("invalid operation");
			}

			if(ast->data.val.op == OP_MOD)	PUSH_R(RDX);
			else				PUSH_R(RAX);

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

static void GenDeclFunc(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(ast->data.type != TP_DECL_FUNC)
		err_exit_msg("node is not a function declaration");
	if(!IS_BINNODE(ast))
		err_exit_msg("invalid declaration");

	node_t *body = ast->child->node, *args = ast->child->next->node;
	LBL("%s", ast->data.val.name);
	ENTER(8*(uint16_t)(body->data.val.id + args->data.val.id));

	GenOpSeq(RIGHT(ast));
}

static void GenAsm(const node_t *ast)
{
	assert(ast);
	assert(ASM_OUT);
	LEAVE_IF_ERR;
	if(!IS_(ASM, ast->data))
		err_exit_msg("is not 'asm'");
	if(!(ast->child && ast->child->node))
		err_exit_msg("invalid node");

	const char *code = ast->child->node->data.val.name;

	char nasm_path[50] = "";
	snprintf(nasm_path, 50, "/tmp/ncc%llu.nasm", _rdtsc());

	/* write nasm to file */
	FILE *nasm = fopen(nasm_path, "w");	assert(nasm);
	fputs("[bits 64]\n", nasm);	fwrite(code, sizeof(char), strlen(code), nasm);
	fclose(nasm);	nasm = NULL;

	/* run nasm */
	pid_t pid = fork();

	if(pid < 0)
		perror("fork");
	else if(pid == 0)
		execv("/bin/nasm", (char *const[]){ "/bin/nasm", "-f", "bin", nasm_path, NULL });
	else
	{
		int status = 0;
		waitpid(pid, &status, 0);

		if(WEXITSTATUS(status))	err_exit_msg("nasm failed");
	}

	/* write elf */
	char *dot = strchr(nasm_path, '.');	assert(dot);
	*dot = '\0';

	int elf_fd = open(nasm_path, O_RDONLY);	assert(elf_fd > 0);
	struct stat elf_finfo = {};		fstat(elf_fd, &elf_finfo);
	char *bin = (char *)mmap(NULL, (size_t)elf_finfo.st_size, PROT_READ, MAP_PRIVATE, elf_fd, 0);
	assert(bin);

	fwrite(bin, 1, (size_t)elf_finfo.st_size, emitter_get_elf());

	munmap(bin, (size_t)elf_finfo.st_size);	bin = NULL;
	close(elf_fd);	elf_fd = 0;
}

static void GenOr(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(OR, ast->data))
		err_exit_msg("is not 'or'");
	if(!IS_BINNODE(ast))
		err_exit_msg("not binary 'or'");

	GenExpr(RIGHT(ast));
	GenExpr(LEFT(ast));

	POP_R(RAX);	POP_R(RDX);
	CMP_RI(RAX, 0);	JNE(".L%lu", LBL_CNT);

	CMP_RI(RDX, 0);	JNE(".L%lu", LBL_CNT);

	PUSH_I(0);	JMP_L(".L%lu", LBL_CNT+1);
	LBL(".L%lu", LBL_CNT);
	PUSH_I(1);
	LBL(".L%lu", LBL_CNT+1);

	LBL_CNT += 2;
}

static void GenAnd(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(AND, ast->data))
		err_exit_msg("is not 'and'");
	if(!IS_BINNODE(ast))
		err_exit_msg("not binary 'and'");

	GenExpr(RIGHT(ast));
	GenExpr(LEFT(ast));

	POP_R(RAX);	POP_R(RDX);

	CMP_RI(RAX, 0);	JE(".L%lu", LBL_CNT);
	CMP_RI(RDX, 0);	JE(".L%lu", LBL_CNT);

	PUSH_I(1);	JMP_L(".L%lu", LBL_CNT+1);

	LBL(".L%lu", LBL_CNT);
	PUSH_I(0);
	LBL(".L%lu", LBL_CNT+1);

	LBL_CNT += 2;
}

static void GenComp(const node_t *ast, const op_t gle)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	
	if(!IS_BINNODE(ast))
		err_exit_msg("node is not binary");
	
	GenExpr(RIGHT(ast));
	GenExpr(LEFT(ast));

	POP_R(RAX);	POP_R(RDX);
	CMP_RR(RAX, RDX);
	switch(gle)
	{
		case OP_GREATER:	JG(".L%lu", LBL_CNT);	break;
		case OP_LESS:		JL(".L%lu", LBL_CNT);	break;
		case OP_EQ:		JE(".L%lu", LBL_CNT);	break;
		case OP_NEQ:		JNE(".L%lu", LBL_CNT);	break;

		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MOD:
		case OP_ASSIGN:
		case OP_OR:
		case OP_AND:
		default:		err_exit_msg("jmptype out of range");
	}

	PUSH_I(0);	JMP_L(".L%lu", LBL_CNT+1);

	LBL(".L%lu", LBL_CNT);
	PUSH_I(1);
	LBL(".L%lu", LBL_CNT+1);

	LBL_CNT += 2;
}

static void GenExpr(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
		
	switch(ast->data.type)
	{
		case TP_NUM:
		case TP_VAR:
		case TP_TAKEADDR:
			GenArifm(ast);
			break;
		case TP_CALL_FUNC:
			GenCallFunc(ast);
			PUSH_R(RAX);
			break;
		case TP_DEREF:
			GenDeref(ast);
			break;
		case TP_OP:
			switch(ast->data.val.op)
			{
				case OP_ADD:
				case OP_SUB:
				case OP_MUL:
				case OP_DIV:
				case OP_MOD:
					GenArifm(ast);
					break;
				case OP_AND:
					GenAnd(ast);
					break;
				case OP_OR:
					GenOr(ast);
					break;
				case OP_GREATER:
				case OP_LESS:
				case OP_EQ:
				case OP_NEQ:
					GenComp(ast, ast->data.val.op);
					break;
				case OP_ASSIGN:
				default:
					err_exit_msg("invalid operation");
			}
			break;

		case TP_LITERAL:	GenStr(ast); break;

		case TP_EOF:
		case TP_ROOT:
		case TP_OP_SEQ:
		case TP_IDENT:
		case TP_PARAM:			/* cannot be expression */
		case TP_KWORD:
		case TP_SYMB:
		case TP_DECL_FUNC:
		default:
			err_exit_msg("type out of range");
	}
}

static void GenIf(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(IF, ast->data))
		err_exit_msg("is not 'if'");

	child_t *child = ast->child;
	if(CHILD_EXISTS(child))
		GenExpr(child->node);
	else
		err_exit_msg("missing condition");
	child = child->next;

	size_t else_lbl = LBL_CNT++, endif_lbl = LBL_CNT++;
	
	POP_R(RAX);
	CMP_RI(RAX, 0);	JE(".L%lu", else_lbl);

	/* 'if' body */
	if(CHILD_EXISTS(child))
		GenOpSeq(child->node);
	else
		err_exit_msg("missing 'if' body");
	child = child->next;

	JMP_L(".L%lu", endif_lbl);
	LBL(".L%lu", else_lbl);

	if(CHILD_EXISTS(child))
		GenOpSeq(child->node);

	/* endif */
	LBL(".L%lu", endif_lbl);
}

static void GenWhile(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(WHILE, ast->data))
		err_exit_msg("is not 'while'");
	if(!IS_BINNODE(ast))
		err_exit_msg("is not binary");

	size_t cond_lbl = LOOP_LBL_CNT++, end_lbl = LOOP_LBL_CNT++;

	LBL(".Lloop%lu", cond_lbl);

	GenExpr(LEFT(ast));

	POP_R(RAX);
	CMP_RI(RAX, 0);	JE(".Lloop%lu", end_lbl);

	GenOpSeq(RIGHT(ast));

	JMP_L(".Lloop%lu", cond_lbl);
	LBL(".Lloop%lu", end_lbl);
}

static void GenAssign(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(!IS_(ASSIGN, ast->data))
		err_exit_msg("is not 'assignment'");
	if(!IS_BINNODE(ast))
		err_exit_msg("is not binary");

	node_t *l_val = LEFT(ast);
	node_t *r_val = RIGHT(ast);

	/* r_val */
	if(r_val->data.type == TP_LITERAL)	GenStr(r_val);
	else					GenExpr(r_val);

	/* l_val */
	if(l_val->data.type == TP_VAR)
	{
		POP_M(RBP, 8*(int32_t)l_val->data.val.id);
	}
	else if(l_val->data.type == TP_DEREF)
	{
		if(!CHILD_EXISTS(l_val->child))
			err_exit_msg("wrong node");

		GenExpr(l_val->child->node);

		POP_R(RAX);
		SHL_RI(RAX, 3);
		POP_M(RAX, 0);
	}
	else
		err_exit_msg("lvalue must be variable or dereference ptr");
}

static void GenCallFunc(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	assert(ASM_OUT);
	if(ast->data.type != TP_CALL_FUNC)
		err_exit_msg("is not a function call");
	if(ast->child == NULL || ast->child->node == NULL)
		err_exit_msg("call node without parameters node");

	node_t *param_node = ast->child->node;

	if(param_node->child)
	{
		child_t *param = param_node->child->prev;

		for(ssize_t i = 0; i < param_node->data.val.id; i++)
		{
			assert(param);

			if(param_node->data.type == TP_LITERAL)	GenStr(param->node);
			else					GenExpr(param->node);

			param = param->prev;
		}
	}

	CALL_L("%s", ast->data.val.name);
	ADD_RI(RSP, 8*(int32_t)param_node->data.val.id);
}

static void GenReturn(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(RETURN, ast->data))
		err_exit_msg("is not a 'return'");
	
	if(ast->child && ast->child->node)
	{
		GenExpr(ast->child->node);
		POP_R(RAX);
	}
	else	MOV_RI(RAX, 0);

	LEAVE;	RET;
}

static void GenBreak(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(BREAK, ast->data))
		err_exit_msg("is not 'break'");

	JMP_L(".Lloop%lu", LOOP_LBL_CNT-1);
}

static void GenContinue(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(!IS_(CONTINUE, ast->data))
		err_exit_msg("is not 'continue'");

	JMP_L(".Lloop%lu", LOOP_LBL_CNT-2);
}

static void GenDeref(const node_t *ast)
{
	LEAVE_IF_ERR;
	assert(ast);
	if(ast->data.type != TP_DEREF)
		err_exit_msg("is not '[]'");
	if(!CHILD_EXISTS(ast->child))
		err_exit_msg("wrong node");

	GenExpr(ast->child->node);

	POP_R(RAX);
	SHL_RI(RAX, 3);
	PUSH_M(RAX, 0);
}
/*---------------------------------------------*/
