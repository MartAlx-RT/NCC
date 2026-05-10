#include "emitter.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

static FILE *NASM = NULL;
static FILE *ELF = NULL;

static refs_t REFS = {};
static lbls_t LBLS = {};

void fixup_add_ref(const ref_t ref)
{
	if(REFS.size >= REFS.cap)
	{
		REFS.refs = (ref_t *)reallocarray(REFS.refs, (REFS.cap *= 2), sizeof(ref));
		assert(REFS.refs);
	}

	REFS.refs[REFS.size++] = ref;
}

void fixup_add_lbl(const lbl_t lbl)
{
	if(LBLS.size >= LBLS.cap)
	{
		LBLS.lbls = (lbl_t *)reallocarray(LBLS.lbls, (LBLS.cap *= 2), sizeof(lbl_t));
		assert(LBLS.lbls);
	}

	LBLS.lbls[LBLS.size++] = lbl;
}

void emitter_init(FILE *nasm, FILE *elf)
{
	assert(nasm);	assert(elf);
	ELF = elf;	NASM = nasm;

	int templ_fd = open("/home/alex/Cprojects/NCC/bin/template", O_RDONLY);
	assert(templ_fd > 0);

	struct stat templ_finfo = {};	fstat(templ_fd, &templ_finfo);
	char *templ = (char *)mmap(NULL, templ_finfo.st_size, PROT_READ, MAP_PRIVATE, templ_fd, 0);
	assert(templ);

	size_t chck_fwrite = fwrite(templ, sizeof(char), templ_finfo.st_size, ELF);

	munmap(templ, templ_finfo.st_size);	templ = NULL;

	if(chck_fwrite != (size_t)templ_finfo.st_size)	{ perror("fwrite"); return; }

	fseek(ELF, 0x138, SEEK_SET);
}

void emit_gavno(void)
{
	write_b('g', 'a', 'v', 'n', 'o');
}

void emit_helloworld(void)
{
	// b83c 0000 00bf 3400 0000 0f05

	write_b(0xb8, 0x3c, 0x00, 0x00, 0x00, 0xbf, 0x34, 0x00, 0x00, 0x00, 0x0f, 0x05);
}

void emit_mov(const mod_t mod, const operand_t dst, const operand_t src, uint32_t disp)
{
	write_b(0x48);	// rex prefix

	switch(mod)
	{
		case MOD_REG_REG:	write_b(0x89, 0xC0 | (src.reg << 3) | (dst.reg));	break;

		case MOD_REG_MEM:
					{
						write_b(0x8b, 0x80 | (dst.reg << 3) | (src.reg));

						bytes32_t bytes = { .num = disp };
						write_b(bytes.bytes[0], bytes.bytes[1], bytes.bytes[2], bytes.bytes[3]);
					}
					break;

					case

	}

	// TODO matan after 22:00
	// TODO all needed emit functions and jumps!!!!
}
