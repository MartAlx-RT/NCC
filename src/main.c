#include "ncc.h"
#include "def_perror.h"
#include <string.h>
#include <malloc.h>

/*-------------------------------------------*/
static inline void PrintUsage(void);
static int Compile(const char *in_filename, const char *out_filename, const char *out_asm_filename,
		const int need_asm, const int need_dump);
/*-------------------------------------------*/

int main(int argc, char *argv[])
{
	char *in_filename = NULL;
	char *out_asm_filename = NULL;
	char *out_filename = NULL;
	int need_asm = 0;
	int need_dump = 0;
	int compile_status = 0;

	for (int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-o"))
		{
			if(++i < argc && out_filename == NULL)
				out_filename = argv[i];
		}
		else if(!strcmp(argv[i], "--asm"))
		{
			need_asm = 1;
			if (++i < argc && out_asm_filename == NULL)
				out_asm_filename = argv[i];
		}
		else if(!strcmp(argv[i], "--help"))
		{
			PrintUsage();
			goto exit;
		}
		else if(!strcmp(argv[i], "--dump"))	need_dump = 1;
		else if (in_filename == NULL)
			in_filename = argv[i];
	}
	
	if(in_filename == NULL)
	{
		PrintUsage();
		compile_status = 1;
		goto exit;
	}

	if(out_filename == NULL)
		out_filename = "a.out";			/* default name */
	if(out_asm_filename == NULL)
		out_asm_filename = "out.nasm";		/* default name */

	compile_status = Compile(in_filename, out_filename, out_asm_filename, need_asm, need_dump);

exit:
//	if(0)	/* turned off */
//	{
//		sleep(1);
//		system("tiv ./Img/funnyded.jpg"); /* easter egg =) */
//	}

	return compile_status;
}

static inline void PrintUsage(void)
{
	fprintf(stderr, colorize("Usage:\t", _BOLD_ _YELLOW_)
						colorize("ncc ", _BOLD_ _GREEN_) colorize("<file>\n", _BOLD_ _MAGENTA_)
						colorize("\t[-o <output filename>]\t", _BOLD_ _CYAN_)
						colorize("Default output filename is 'a.out'\n", _GREEN_)
						colorize("\t[--asm <asm filename>]\t", _BOLD_ _CYAN_)
						colorize("Will be produced if this flag typed\n", _GREEN_)
						colorize("\t\t[--help]\t", _BOLD_ _CYAN_)
						colorize("display this information\n\n", _GREEN_));
}

#define SYS_MSG_SIZE 100
static int Compile(const char *in_filename, const char *out_filename, const char *out_asm_filename,
		const int need_asm, const int need_dump)
{
	assert(in_filename);
	assert(out_filename);
	assert(out_asm_filename);

	FILE *output_asm = NULL;
	char *code = NULL;
	toks_t *toks = NULL;
	node_t *tree = NULL;
	char sys_msg[SYS_MSG_SIZE] = "";

	snprintf(sys_msg, SYS_MSG_SIZE, "myasm %s -o %s", out_asm_filename, out_filename);

	output_asm = fopen(out_asm_filename, "w");
	if (output_asm == NULL)
	{
		print_err_msg("output file(s) cannot be open");
		goto err_exit;
	}

	code = NULL;
	if(ReadFileToBuf(in_filename, &code) == 0)
		goto err_exit;
	assert(code);

	toks = Tokenize(code);
	if(toks == NULL || toks->data == NULL)
	{
		print_err_msg("tokenize failed");
		goto err_exit;
	}

	tree = Parse(toks, in_filename);
	if(tree == NULL)
	{
		print_err_msg("parse failed");
		goto err_exit;
	}

	if(need_dump)	TreeDump(tree);
	
	if(CompileTree(tree, output_asm))
	{
		print_err_msg("backend failed");
		goto err_exit;
	}
	
	fclose(output_asm);
	output_asm = NULL;
	
//	if (system(sys_msg))
//		goto err_exit;

	/*---------------------*/
	free(code);
	ToksDestroy(toks);
	TreeDestroy(tree);
	if (!need_asm)
		remove(out_asm_filename);
	return 0;

err_exit:
	free(code);
	ToksDestroy(toks);
	TreeDestroy(tree);
	if(!need_asm)
		remove(out_filename);
	if(output_asm)
		fclose(output_asm);
	if (output_asm && !need_asm)
		remove(out_asm_filename);
	return 1;
}
#undef SYS_MSG_SIZE

