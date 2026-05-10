#include "emitter.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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

void emit_mov(const mov_t type, const mod_t mod, const reg_t reg, const operand_t op, const uint32_t disp)
{
	write_b(0x48);

	if(type == MOV_IMM_TO_REG)
	{
		const bytes64_t conv = { .num = op.imm };
		write_b(type+reg, conv.bytes[0], conv.bytes[1], conv.bytes[2], conv.bytes[3],
				conv.bytes[4], conv.bytes[5], conv.bytes[6], conv.bytes[7]);
	}
	else if(type == MOV_MEM_TO_REG || type == MOV_REG_TO_MEM)
	{
		write_b(type, (mod<<6)|(reg<<3)|(op.reg));

		if(mod == MOD_M_DISP8)		write_b(disp);
		else if(mod == MOD_M_DISP32)
		{
			const bytes32_t conv = { .num = disp };
			write_b(conv.bytes[0], conv.bytes[1], conv.bytes[2], conv.bytes[3]);
		}
	}
	else	fprintf(stderr, "type out of range\n");
}

void emit_push(const push_t type, const operand_t op)
{
	switch(type)
	{
		case PUSH_REG:	write_b(type + op.reg);				break;
		case PUSH_MEM:	write_b(type, (MOD_M<<6)|(6<<3)|(op.reg));	break;

		case PUSH_IMM:
				{
					const bytes32_t conv = { .num = op.imm };
					write_b(0x68, conv.bytes[0], conv.bytes[1], conv.bytes[2], conv.bytes[3]);
				}
				break;

		default:	fprintf(stderr, "type out of range\n");
	}
}

void emit_enter(const uint16_t shift)
{
	const bytes16_t conv = { .num = shift };

	write_b(0xc8, conv.bytes[0], conv.bytes[1], 0x00);
}

void emit_leave(void)
{
	write_b(0xc9);
}

void emit_ret(void)
{
	write_b(0xc3);	// near
			// far = 0xcb
}

void emit_syscall(void)
{
	write_b(0x0f, 0x05);
}
