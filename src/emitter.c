#include "emitter.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

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
	char *templ = (char *)mmap(NULL, templ_finfo.st_size, PROT_READ, MAP_PRIVATE, templ_fd, 0);
	assert(templ);

	size_t chck_fwrite = fwrite(templ, sizeof(char), templ_finfo.st_size, ELF);

	munmap(templ, templ_finfo.st_size);	close(templ_fd);
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

size_t emitter_get_elf_pos(void)
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
			size_t abs_distance = (distance > 0)? distance : -distance;
			if(abs_distance >= INT32_MAX)
				fprintf(stderr, "`%s`: distance is too large\n", lbl->name);

			if(ref->type == FIX_JMP)	{ write_b(JMP_REL32);	write_d((int32_t)distance); }
			else if(ref->type == FIX_CALL)	{ write_b(CALL_REL32);	write_d(distance); }
			else				fprintf(stderr, "ref type out of range\n");
		}
	}

	elf_seek_end();
}

/* emit instruction functions */

void emit_mov(const mov_t type, const mod_t mod, const reg_t reg, const operand_t op, const uint32_t disp)
{
	write_b(0x48);

	if(type == MOV_IMM_TO_REG)
	{
		write_b(type + reg);	write_q(op.imm);
	}
	else if(type == MOV_MEM_TO_REG || type == MOV_REG_TO_MEM)
	{
		write_b(type, (mod<<6)|(reg<<3)|(op.reg));

		if(mod == MOD_M_DISP8)		write_b(disp);
		else if(mod == MOD_M_DISP32)	write_d(disp);
	}
	else	fprintf(stderr, "mov type out of range\n");
}

void emit_push(const push_t type, const operand_t op)
{
	switch(type)
	{
		case PUSH_REG:	write_b(type + op.reg);				break;
		case PUSH_MEM:	write_b(type, (MOD_M<<6)|(6<<3)|(op.reg));	break;
		case PUSH_IMM:	write_b(type);	write_d(op.imm);		break;

		default:	fprintf(stderr, "push type out of range\n");
	}
}

void emit_abs_branch(const branch_t branch, const operand_t op)
{
	assert(branch == JMP_ABS || branch == CALL_ABS);

	write_b(branch, (MOD_R<<6)|(4<<3)|(op.reg));
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

