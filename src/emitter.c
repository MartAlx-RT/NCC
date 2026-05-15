#include "emitter.h"
#include "def_emitters.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>

#pragma GCC push_options
#pragma GCC optimize ("-fno-stack-protector")

#define MODRM(mod, r, rm)	((uint8_t)((mod<<6)|(r<<3)|(rm)))

// TODO
// make code pretty
// add shl, shr
// add other needed emits
//

static FILE *NASM = NULL;
static FILE *ELF = NULL;
static fixups_t FIXUPS = {};

/* common functions */ 

void emitter_init(FILE *nasm, FILE *elf)
{
	assert(nasm);	assert(elf);
	ELF = elf;	NASM = nasm;

	int templ_fd = open("/home/alex/Cprojects/NCC/bin/template", O_RDONLY);
	assert(templ_fd > 0);

	struct stat templ_finfo = {};	fstat(templ_fd, &templ_finfo);
	char *templ = (char *)mmap(NULL, (size_t)templ_finfo.st_size, PROT_READ, MAP_PRIVATE, templ_fd, 0);
	assert(templ);

	size_t chck_fwrite = fwrite(templ, sizeof(char), (size_t)templ_finfo.st_size, ELF);

	munmap(templ, (size_t)templ_finfo.st_size);	close(templ_fd);
	templ = 0;	templ_fd = 0;

	if(chck_fwrite != (size_t)templ_finfo.st_size)	{ perror("fwrite"); return; }

	fseek(ELF, 0x138, SEEK_SET);
}

void emitter_deinit(void)
{
	free(FIXUPS.refs.refs);	FIXUPS.refs.refs = NULL;
	free(FIXUPS.lbls.lbls);	FIXUPS.lbls.lbls = NULL;

	FIXUPS.refs.cap = FIXUPS.refs.size = 0;
	FIXUPS.lbls.cap = FIXUPS.lbls.size = 0;

	NASM = ELF = NULL;
}

ssize_t emitter_get_elf_pos(void)
{
	return ftell(ELF);
}

void emitter_fixup_add_ref(const ref_t ref)
{
	if(FIXUPS.refs.size >= FIXUPS.refs.cap)
	{
		FIXUPS.refs.cap = FIXUPS.refs.cap*2 + 1;
		FIXUPS.refs.refs = (ref_t *)reallocarray(FIXUPS.refs.refs, FIXUPS.refs.cap, sizeof(ref_t));
		assert(FIXUPS.refs.refs);
	}

	FIXUPS.refs.refs[FIXUPS.refs.size++] = ref;
}

void emitter_fixup_add_lbl(const lbl_t lbl)
{
	if(FIXUPS.lbls.size >= FIXUPS.lbls.cap)
	{
		FIXUPS.lbls.cap = FIXUPS.lbls.cap*2 + 1;
		FIXUPS.lbls.lbls = (lbl_t *)reallocarray(FIXUPS.lbls.lbls, FIXUPS.lbls.cap, sizeof(lbl_t));
		assert(FIXUPS.lbls.lbls);
	}

	FIXUPS.lbls.lbls[FIXUPS.lbls.size++] = lbl;
}

void emitter_fixup(void)
{
	for(size_t i = 0; i < FIXUPS.refs.size; i++)
	{
		lbl_t *lbl = NULL;
		ref_t *ref = &FIXUPS.refs.refs[i];

		for(size_t l = 0; l < FIXUPS.lbls.size; l++)
		{
			if(!strcmp(FIXUPS.lbls.lbls[l].name, ref->name))
				lbl = &FIXUPS.lbls.lbls[l];
		}

		if(lbl == NULL)	fprintf(stderr, "unknown label: `%s`\n", ref->name);
		else
		{
			elf_seek(ref->pos - 5);

			ssize_t distance = lbl->pos - ref->pos;
			size_t abs_distance = (size_t) ((distance > 0)? distance : -distance);
			if(abs_distance >= INT32_MAX)
				fprintf(stderr, "`%s`: distance is too large\n", lbl->name);

			if(ref->type == FIX_JMP)	write_b(JMP_REL32);
			else if(ref->type == FIX_CALL)	write_b(CALL_REL32);
			else				fprintf(stderr, "ref type out of range\n");

			write_d((uint32_t)(int32_t)distance);
		}
	}

	elf_seek_end();
}

/* emit instruction functions */

void emit_mov(const mov_t type, const mod_t mod, const reg_t reg, const operand_t op, const uint32_t disp)
{
	write_b(REX_W);

	switch(type)
	{
		case MOV_IMM_TO_REG:	write_b((uint8_t)(type + reg));	write_q(op.imm);	break;

		case MOV_REG_TO_MEM:
		case MOV_MEM_TO_REG:
					write_b(type, MODRM(mod, reg, op.reg));
					if(mod == MOD_M_DISP)	write_d(disp);
					break;

		case MOV_IMM_TO_MEM:
		default:		fprintf(stderr, "mov type out of range\n");
	}
}

void emit_push(const push_t type, mod_t mod, const operand_t op, uint32_t disp)
{
	switch(type)
	{
		case PUSH_REG:	write_b((uint8_t)(PUSH_REG + op.reg));			break;
		case PUSH_IMM:	write_b(PUSH_IMM);	write_d((uint32_t)op.imm);	break;

		case PUSH_MEM:
				write_b(PUSH_MEM, MODRM(mod, 6, op.reg));
				if(mod == MOD_M_DISP)	write_d(disp);
				break;

		default:	fprintf(stderr, "push type out of range\n");
	}
}

void emit_pop(const pop_t type, mod_t mod, const operand_t op, uint32_t disp)
{
	switch(type)
	{
		case POP_REG:	write_b((uint8_t)(POP_REG + op.reg));	break;

		case POP_MEM:
				write_b(POP_MEM, MODRM(mod, 0, op.reg));
				if(mod == MOD_M_DISP)	write_d(disp);
				break;

		default:	fprintf(stderr, "pop type out of range\n");
	}
}

void emit_abs_branch(const branch_t branch, const operand_t op)
{
	write_b((uint8_t)branch, MODRM(6, 4, op.reg));
}

void emit_rel_branch(const fixup_type_t type, const char *lbl)
{
	assert(lbl);

	write_b(0x90, 0x90, 0x90, 0x90, 0x90);
	emitter_fixup_add_ref((const ref_t){ .pos = emitter_get_elf_pos(), .type = type, .name = lbl });
}

void emit_lbl(const char *lbl)
{
	assert(lbl);

	emitter_fixup_add_lbl((const lbl_t){ .pos = emitter_get_elf_pos(), .name = lbl });
}

void emit_enter(const uint16_t shift)
{
	write_b(0xc8);	write_w(shift);
}

void emit_leave(void)
{
	write_b(0xc9);
}

void emit_ret(void)
{
	write_b(0xc3);
}

void emit_syscall(void)
{
	write_b(0x0f, 0x05);
}

void emit_arifm(const arifm_t type, operand_t dst, operand_t src)
{
	// ADD_REG_TO_REG	= 0x3
	// SUB_REG_TO_REG	= 0x2b
	// IMUL_REG_TO_REG	= 0xaf0f
	// IDIV_REG_TO_REG	= 0xf7
	// CMP_REG_TO_REG	= 0x3b
	// SHL_IMM_TO_REG	= 0xc1
	// SHR_IMM_TO_REG	= 0xc1

	write_b(REX_W);

	switch(type)
	{
		case ARIFM_ADD_REG_TO_REG:
		case ARIFM_SUB_REG_TO_REG:
		case ARIFM_CMP_REG_TO_REG:
			write_b((uint8_t)type, MODRM(MOD_R, dst.reg, src.reg));
			break;

		case ARIFM_SHL_IMM_TO_REG:
			write_b(0xc1, MODRM(MOD_R, 4, dst.reg), (uint8_t)src.imm);
			break;
		case ARIFM_SHR_IMM_TO_REG:
			write_b(0xc1, MODRM(MOD_R, 5, dst.reg), (uint8_t)src.imm);
			break;

		case ARIFM_IMUL_REG_TO_REG:
			write_b(0x0f, 0xaf, MODRM(MOD_R, dst.reg, src.reg));
			break;
		case ARIFM_IDIV_REG:
			write_b(0xf7, MODRM(MOD_R, 7, src.reg));
			break;

		default:	fprintf(stderr, "arifm type out of range\n");
	}
}

#pragma GCC pop_options

