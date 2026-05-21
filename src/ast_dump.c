#include "ncc.h"
#include "def_perror.h"
#include <sys/wait.h>
#include <unistd.h>
/*---------------------------------------------------------*/
static const char *NODE_TYPE_NAME[] =
	{"eof", "root", "number", "operation", "op. sequence", "identifier", "parameters", "variable", "keyword", "symbol", "decl function", "call function", "literal", "&", "[]"};
static const char *OP_NAME[] =
	{"+", "-", "*", "/", "\\>", "\\<", "=", "==", "or", "and", "!=", "%", ">=", "<="};
static const char *KWORD_NAME[] =
	{"if", "else", "while", "for", "continue", "break", "return", "pass", "asm", "func"};
static const char *SYMB_NAME[] =
	{"{", "}", "(", ")", ";", ",", "\"", "[", "]"};

/*---------------------------------------------------------*/
static void PrintNodeData(const node_data_t data, FILE *out_file);
static void PrintDigraphNode(const node_t *node, FILE *dot_file, size_t *call_count);
static void PrintDigraphTree(const node_t *tree, FILE *dot_file);
static void CreateDigraph(const node_t *tree, FILE *f);
/*---------------------------------------------------------*/

static void PrintNodeData(const node_data_t data, FILE *out_file)
{
	assert(out_file);

	const char *data_s = NULL;
	long data_num = 0;

	switch(data.type)
	{
		case TP_EOF:
				data_s = "EOF";
				break;
		case TP_KWORD:
				data_s = KWORD_NAME[(int)data.val.kword];
				break;
		case TP_NUM:
				data_s = NULL;
				data_num = data.val.num;
				break;
		case TP_OP:
				data_s = OP_NAME[(int)data.val.op];
				break;
		case TP_SYMB:
				data_s = SYMB_NAME[(int)data.val.symb];
				break;
		case TP_VAR:
		case TP_PARAM:
		case TP_TAKEADDR:
		case TP_OP_SEQ:
				data_s = NULL;
				data_num = data.val.id;
				break;
		case TP_CALL_FUNC:
		case TP_DECL_FUNC:
		case TP_LITERAL:
		case TP_IDENT:
				data_s = data.val.name;
				break;
		case TP_DEREF:
		case TP_ROOT:
				break;
		default:
				data_s = "??";
				break;
	}
	
	if(data_s)
		fprintf(out_file, "%s", data_s);
	else
		fprintf(out_file, "%ld", data_num);
}

static void PrintDigraphNode(const node_t *node, FILE *dot_file, size_t *call_count)
{
	assert(call_count);
	assert(dot_file);
	assert(node);

	if ((*call_count)++ > MAX_REC_DEPTH)
	{
		print_err_msg("tree overflow");
		return;
	}
	
	/*------------------------------------*/

	fprintf(dot_file, "label%lu[shape=record, style=\"rounded, filled\", fillcolor=\"#a8daf0ff\", label=\"{ node[%p] | type = %s | val = ",
			(size_t)node, node, NODE_TYPE_NAME[node->data.type]);
	PrintNodeData(node->data, dot_file);
	fprintf(dot_file, " | prnt[%p]}\"];\n", node->parent);
	
	if(node->child)
	{
		child_t *child = node->child;
		while (child)
		{
			if(child->node && child->node->parent == node)
				fprintf(dot_file, "label%lu->label%lu [color=purple, dir=both]\n", (size_t)node, (size_t)child->node);
			else
				fprintf(dot_file, "label%lu->label%lu [color=red]\n", (size_t)node, (size_t)child->node);

			PrintDigraphNode(child->node, dot_file, call_count);
			child = child->next;
		}
	}
}

static void PrintDigraphTree(const node_t *tree, FILE *dot_file)
{
	assert(tree);
	assert(dot_file);
	
	size_t call_count = 0;
	PrintDigraphNode(tree, dot_file, &call_count);
}

static void CreateDigraph(const node_t *tree, FILE *f)
{
	assert(f);
	assert(tree);

	/*--------------------------------------*/

	fprintf(f, "digraph AST\n{\n");
	PrintDigraphTree(tree, f);
	putc('}', f);
	
	fclose(f);
}

#define DOT_PATH "/tmp/ncc_dot_dot_dump.dot"
void TreeDump(const node_t *tree)
{	
	assert(tree);

	FILE *dot_file = fopen(DOT_PATH, "w");	assert(dot_file);
	CreateDigraph(tree, dot_file);

	pid_t pid = fork();

	if(pid < 0)
		perror("fork");
	else if(pid == 0)
		execv("/bin/dot", (char *const[]){ "bin/dot", DOT_PATH, "-Tsvg", "-o", "dump.svg", NULL });
	else
	{
		int status = 0;
		waitpid(pid, &status, 0);

		if(WEXITSTATUS(status))	fprintf(stderr, "dump failed\n");
	}	
}

void PrintToks(node_data_t data[], FILE *dump_file, size_t limit)
{
	if(data == NULL || dump_file == NULL)
	{
		print_err_msg("nullptr passed as argument(s)");
		return;
	}
	
	for (size_t i = 0; i < limit; i++)
	{
		fprintf(dump_file, "[%lu]\t", i);
		switch (data[i].type)
		{
		case TP_IDENT:
			fprintf(dump_file, "ident {%s}", data[i].val.name);
			break;
		case TP_KWORD:
			fprintf(dump_file, "kword %s", KWORD_NAME[(int)data[i].val.kword]);
			break;
		case TP_NUM:
			fprintf(dump_file, "num %ld", data[i].val.num);
			break;
		case TP_OP:
			fprintf(dump_file, "op %s", OP_NAME[(int)data[i].val.op]);
			break;
		case TP_SYMB:
			fprintf(dump_file, "symb %s", SYMB_NAME[(int)data[i].val.symb]);
			break;
		case TP_LITERAL:
			fprintf(dump_file, "lit {%s}", data[i].val.name);
			break;
		case TP_TAKEADDR:
			fprintf(dump_file, "takeaddr (&)");
			break;
		case TP_EOF:
			fprintf(dump_file, "EOF\n");
			return;
		case TP_OP_SEQ:
		case TP_PARAM:
		case TP_ROOT:
		case TP_DECL_FUNC:
		case TP_CALL_FUNC:
		case TP_VAR:
		case TP_DEREF:
		default:
			print_err_msg("datatype is out of range 'node_type_t'");
			break;
		}
		fputc('\n', dump_file);
	}
}

