#include "ncc.h"
#include "def_perror.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/*-------------------------------------------*/
static inline void PrintUsage(void);
static int Compile(const char *code_path, const char *elf_path, const char *nasm_path, const char *dump_path);
/*-------------------------------------------*/

int main(int argc, char *argv[])
{
	int compile_status = 0;
	char *code_path = NULL, *elf_path = NULL, *nasm_path = NULL, *dump_path = NULL;

	for (int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-o"))
		{
			if(++i < argc)	elf_path = argv[i];
		}
		else if(!strcmp(argv[i], "--nasm"))
		{
			if(++i < argc)	nasm_path = argv[i];
		}
		else if(!strcmp(argv[i], "--help"))
		{
			PrintUsage();
			goto exit;
		}
		else if(!strcmp(argv[i], "--dump"))
		{
			if(++i < argc)	dump_path = argv[i];
		}
		else if(code_path == NULL)	code_path = argv[i];
		else
			fprintf(stderr, "invalid cl args, try --help to get usage\n");
	}
	
	if(code_path == NULL)
	{
		fprintf(stderr, "missing filename\n");
		compile_status = 1;
		goto exit;
	}

	if(elf_path == NULL)	elf_path = "a.out";
	if(nasm_path == NULL)	nasm_path = "out.nasm";

	compile_status = Compile(code_path, elf_path, nasm_path, dump_path);

exit:
	return compile_status;
}

static inline void PrintUsage(void)
{
	fprintf(stderr, colorize("Usage:\t", _BOLD_ _YELLOW_)
						colorize("ncc ", _BOLD_ _GREEN_) colorize("<file>\n", _BOLD_ _MAGENTA_)
						colorize("\t[-o <output filename>]\t", _BOLD_ _CYAN_)
						colorize("Default output filename is 'a.out'\n", _GREEN_)
						colorize("\t[--nasm <asm filename>]\t", _BOLD_ _CYAN_)
						colorize("Will be produced if this flag typed\n", _GREEN_)
						colorize("\t\t[--help]\t", _BOLD_ _CYAN_)
						colorize("display this information\n\n", _GREEN_));
}

static int Compile(const char *code_path, const char *elf_path, const char *nasm_path, const char *dump_path)
{
	assert(code_path);	assert(elf_path);

	int compile_status = 0;
	FILE *nasm = NULL, *elf = NULL;
	char *code = NULL;
	toks_t *toks = NULL;
	node_t *ast = NULL;

	int code_fd = open(code_path, O_RDONLY);
	if(code_fd < 0)	{ perror("open"); return 1; }
	struct stat code_finfo = {};	fstat(code_fd, &code_finfo);

	code = (char *)mmap(NULL, (size_t)code_finfo.st_size, PROT_READ, MAP_PRIVATE, code_fd, 0);
	assert(code);

	elf = fopen(elf_path, "wb");
	if(elf == NULL)	{ perror("fopen"); return 1; }

	if(nasm_path)
	{
		nasm = fopen(nasm_path, "w");
		if(nasm == NULL) { perror("fopen"); return 1; }
	}
	else
	{
		nasm = tmpfile();
		if(nasm == NULL) { perror("tmpfile"); return 1; }
	}


	toks = Tokenize(code);
	if(toks == NULL || toks->data == NULL)
	{
		print_err_msg("tokenize failed");
		goto err_exit;
	}
	munmap(code, (size_t)code_finfo.st_size);
	code = NULL;

	ast = Parse(toks, code_path);
	if(ast == NULL)
	{
		print_err_msg("parse failed");
		goto err_exit;
	}

	if(dump_path)
	{
		FILE *dump = fopen(dump_path, "w");
		if(dump)
		{
			TreeDump(ast, (char *)dump_path);
			fclose(dump);
		}
		else	perror("fopen");
	}
	
	if(CompileTree(ast, elf, nasm))
	{
		print_err_msg("backend failed");
		goto err_exit;
	}

	goto normal_exit;

err_exit:
	compile_status = 1;
	
normal_exit:
	TreeDestroy(ast);	ast = NULL;
	ToksDestroy(toks);	toks = NULL;

	fclose(elf);		elf = NULL;
	fclose(nasm);		nasm = NULL;

	return compile_status;
}

