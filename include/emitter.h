#include <fcntl.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>

#define write_nasm(code, ...)	\
	do { assert(NASM); fprintf(NASM, code, ##__VA_ARGS__); } while(0)

#define write_elf(type, ...)								\
	do										\
	{										\
		assert(ELF);								\
		fwrite(&(const type[]){ __VA_ARGS__ },					\
				sizeof(type),						\
				sizeof((const type[]){ __VA_ARGS__ })/sizeof(type),	\
				ELF);							\
	} while(0)

#define write_b(...)	write_elf(uint8_t, __VA_ARGS__)
#define write_w(...)	write_elf(uint16_t, __VA_ARGS__)
#define write_d(...)	write_elf(uint32_t, __VA_ARGS__)
#define write_q(...)	write_elf(uint64_t, __VA_ARGS__)

#define write_n(type, val, n)					\
	do							\
	{							\
		assert(ELF);					\
		fwrite(&(type){ val }, sizeof(type), n, ELF);	\
	} while(0)

#define crnt_pos	ftell(ELF)

typedef enum fixup_type_t
{
	FIX_BYTE,
	FIX_WORD,
	FIX_DWORD,
	FIX_QWORD
} fixup_type_t;

typedef struct ref_t
{
	size_t pos;
	fixup_type_t type;
	const char *name;
} ref_t;

typedef struct refs_t
{
	ref_t *refs;
	size_t size, cap;
} refs_t;

typedef struct lbl_t
{
	size_t pos;
	const char *name;
} lbl_t;

typedef struct lbls_t
{
	lbl_t *lbls;
	size_t size, cap;
} lbls_t;

