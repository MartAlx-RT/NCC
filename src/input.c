#include "ncc.h"
#include "def_perror.h"
#include <sys/stat.h>
#include <malloc.h>

size_t ReadFileToBuf(const char *file_path, char **buf)
{
    if(file_path == NULL || buf == NULL)
	{
		print_err_msg("nullptr passed as argument(s)");
		return 0;
	}

	FILE *file = fopen(file_path, "r");
    if(file == NULL)
    {
		print_err_msg("input file cannot be open");
		return 0;
	}

    struct stat file_info = {};
    if(stat(file_path, &file_info))
    {
		print_err_msg("invalid file");
		return 0;
	}
    
    size_t buf_size = (size_t)file_info.st_size;

    *buf = (char *)calloc(buf_size + 2, sizeof(char));
	if(*buf == NULL)
	{
		print_err_msg("buffer overflow");
		return 0;
	}

	if(fread(*buf, sizeof(char), buf_size, file) != buf_size)
    {
		print_err_msg("fread failed");
		return 0;
	}

	fclose(file);

    (*buf)[buf_size] = '\0';
    (*buf)[buf_size + 1] = EOF;

    return buf_size;
}

