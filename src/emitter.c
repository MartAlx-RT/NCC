#include "emitter.h"
#include "def_emitters.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <elf.h>

#pragma GCC push_options
#pragma GCC optimize ("-fno-stack-protector")

#define GEN_MODRM(mod, r, rm)	((uint8_t)((mod<<6)|(r<<3)|(rm)))

static FILE *NASM = NULL;
static FILE *ELF = NULL;
static fixups_t FIXUPS = {};

/* common functions */ 

/* initializes file ptrs, write elf header */
void emitter_init(FILE *elf, FILE *nasm)
{
	assert(elf);	assert(nasm);
	ELF = elf;	NASM = nasm;

	const Elf64_Ehdr elf_header =
	{
		.e_ident =
		{
			[EI_MAG0] = ELFMAG0,		/* elf magic nums */
			[EI_MAG1] = ELFMAG1,
			[EI_MAG2] = ELFMAG2,
			[EI_MAG3] = ELFMAG3,

			[EI_CLASS] = ELFCLASS64,	/* bits */
			[EI_DATA] = ELFDATA2LSB,	/* endian */
			[EI_VERSION] = EV_CURRENT,	/* crnt version */
			[EI_OSABI] = ELFOSABI_SYSV,	/* OS ABI */
			[EI_ABIVERSION] = 0,
			[EI_PAD] = 0
		},

		.e_flags = 0,				/* rofl flags */
		.e_type = ET_EXEC,
		.e_machine = EM_X86_64,
		.e_version = EV_CURRENT,

		.e_entry = ELF_ENTRY_VA,
		.e_phoff = sizeof(Elf64_Ehdr),		/* prog hdr off */
		.e_shoff = 0,				/* section hdr tbl off */

		.e_ehsize = sizeof(Elf64_Ehdr),		/* elf hdr size */
		.e_phentsize = sizeof(Elf64_Phdr),	/* prog hdr size */
		.e_shentsize = sizeof(Elf64_Shdr),

		.e_phnum = 1,				/* number of prog hdrs */
		.e_shnum = 0,				/* number of sect hdrs */
		.e_shstrndx = SHN_UNDEF			/* index of tbl of sections' names */
	};

	const Elf64_Phdr program_header =
	{
		.p_type = PT_LOAD,
		.p_offset = ELF_START_OFF,		/* file pos off */
		.p_vaddr = ELF_ENTRY_VA,		/* entry virtual addr */
		.p_paddr = 0xDED,			/* rofl addr */
                                                	
		.p_filesz = 0,				/* needed fixup */
		.p_memsz = 0,				/* needed fixup */

		.p_flags = PF_R | PF_X,			/* section rwx */
		.p_align = PAGE_SIZE			/* algnment */
	};

	/* write header to a file */
	fwrite(&elf_header, sizeof(elf_header), 1, ELF);
	fwrite(&program_header, sizeof(program_header), 1, ELF);

	fseek(emitter_get_elf(), ELF_START_OFF, SEEK_SET);
}

/* frees alloced, fixups elf header */
void emitter_deinit(void)
{
	for(size_t i = 0; i < FIXUPS.lbls.size; i++)
		free(FIXUPS.lbls.lbls[i].name);

	for(size_t i = 0; i < FIXUPS.refs.size; i++)
		free(FIXUPS.refs.refs[i].name);

	free(FIXUPS.refs.refs);	FIXUPS.refs.refs = NULL;
	free(FIXUPS.lbls.lbls);	FIXUPS.lbls.lbls = NULL;

	FIXUPS.refs.cap = FIXUPS.refs.size = 0;
	FIXUPS.lbls.cap = FIXUPS.lbls.size = 0;

	const size_t elf_size = (size_t)emitter_get_elf_pos();
	fseek(emitter_get_elf(),
		sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) - 3*sizeof(uint64_t),
		SEEK_SET);

	write_q(elf_size, elf_size);
	fseek(emitter_get_elf(), 0, SEEK_END);

	NASM = ELF = NULL;
}

ssize_t emitter_get_elf_pos(void)
{
	return ftell(ELF);
}

FILE *emitter_get_elf(void)
{
	return ELF;
}

/* adds reference to label */
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

/* creates formatted string */
char *emitter_make_msg(const char *fmt, va_list args)
{
	va_list vprintf_args;

	va_copy(vprintf_args, args);
	int n = vsnprintf(NULL, 0, fmt, vprintf_args);
	if(n < 0)	return NULL;

	size_t size = (size_t)n;

	char *msg = (char *)calloc(size+1, sizeof(char));
	assert(msg);

	va_copy(vprintf_args, args);
	vsnprintf(msg, size+1, fmt, vprintf_args);

	return msg;
}

/* add label */
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

/* fixups instruction with labels */
void emitter_fixup(void)
{
	const ssize_t current_pos = emitter_get_elf_pos();

	for(size_t i = 0; i < FIXUPS.refs.size; i++)
	{
		lbl_t *lbl = NULL;
		ref_t *ref = &FIXUPS.refs.refs[i];

		for(size_t l = 0; l < FIXUPS.lbls.size; l++)
		{
			if(!strcmp(FIXUPS.lbls.lbls[l].name, ref->name))
				lbl = &FIXUPS.lbls.lbls[l];
		}

		if(lbl == NULL)
		{
			fprintf(stderr, "unknown label: `%s`\n", ref->name);
			// TODO: warning -> error
		}
		else
		{
			ssize_t distance = lbl->pos - ref->pos;
			size_t abs_distance = (size_t) ((distance > 0)? distance : -distance);
			if(abs_distance >= INT32_MAX)
				fprintf(stderr, "`%s`: distance is too large\n", lbl->name);

			if(ref->type == B_CALL_REL || ref->type == B_JMP_REL)
				fseek(emitter_get_elf(), ref->pos - 5, SEEK_SET);
			else
			{
				fseek(emitter_get_elf(), ref->pos - 6, SEEK_SET);
				write_b(0x0f);
			}
			write_b(ref->type);	write_d((uint32_t)distance);
		}
	}

	fseek(emitter_get_elf(), current_pos, SEEK_SET);
}

/* emit instruction functions */

void emit_mov(const mov_t type, const mod_t mod, const reg_t reg, const operand_t op, const int32_t disp)
{
	write_b(REX_W);

	switch(type)
	{
		case MOV_IMM_TO_REG:
					write_b((uint8_t)(type + reg));
					write_q((uint64_t)op.imm);
					break;

		case MOV_REG_TO_MEM:
		case MOV_MEM_TO_REG:
					write_b(type, GEN_MODRM(mod, reg, op.reg));
					if(mod == MOD_M_DISP)	write_d((uint32_t)disp);
					break;

		case MOV_IMM_TO_MEM:
		default:		fprintf(stderr, "mov type out of range\n");
	}
}

void emit_push(const push_t type, mod_t mod, const operand_t op, const int32_t disp)
{
	switch(type)
	{
		case PUSH_REG:	write_b((uint8_t)(PUSH_REG + op.reg));			break;
		case PUSH_IMM:	write_b(PUSH_IMM);	write_d((uint32_t)op.imm);	break;

		case PUSH_MEM:
				write_b(PUSH_MEM, GEN_MODRM(mod, 6, op.reg));
				if(mod == MOD_M_DISP)	write_d((uint32_t)disp);
				break;

		default:	fprintf(stderr, "push type out of range\n");
	}
}

void emit_pop(const pop_t type, mod_t mod, const operand_t op, const int32_t disp)
{
	switch(type)
	{
		case POP_REG:	write_b((uint8_t)(POP_REG + op.reg));	break;

		case POP_MEM:
				write_b(POP_MEM, GEN_MODRM(mod, 0, op.reg));
				if(mod == MOD_M_DISP)	write_d((uint32_t)disp);
				break;

		default:	fprintf(stderr, "pop type out of range\n");
	}
}

void emit_abs_branch(const branch_t type, const operand_t op)
{
	write_b((uint8_t)type, GEN_MODRM(6, 4, op.reg));
}

void emit_rel_branch(const branch_t type, const char *fmt, ...)
{
	assert(fmt);

	write_b(0x90, 0x90, 0x90, 0x90, 0x90, 0x90);

	ref_t ref = {};
	ref.pos = emitter_get_elf_pos();
	ref.type = type;

	va_list args;
	va_start(args, fmt);
	ref.name = emitter_make_msg(fmt, args);
	va_end(args);

	emitter_fixup_add_ref(ref);
}

void emit_lbl(const char *fmt, ...)
{
	assert(fmt);

	lbl_t lbl = {};
	lbl.pos = emitter_get_elf_pos();

	va_list args;
	va_start(args, fmt);
	lbl.name = emitter_make_msg(fmt, args);
	va_end(args);

	emitter_fixup_add_lbl(lbl);
}

void emit_enter(const uint16_t shift)
{
	write_b(0xc8);
	write_w(shift);	write_b(0x00);
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
			write_b((uint8_t)type, GEN_MODRM(MOD_R, dst.reg, src.reg));
			break;

		case ARIFM_CMP_REG_TO_IMM:
			write_b(ARIFM_CMP_REG_TO_IMM, GEN_MODRM(MOD_R, 7, dst.reg));
			write_d((uint32_t)src.imm);
			break;

		/* without unique opcode */
		case ARIFM_ADD_IMM_TO_REG:
			write_b(0x83, GEN_MODRM(MOD_R, 0, dst.reg), (uint8_t)src.imm);
			break;
		case ARIFM_SUB_IMM_TO_REG:
			write_b(0x83, GEN_MODRM(MOD_R, 5, dst.reg), (uint8_t)src.imm);
			break;
		case ARIFM_SHL_IMM_TO_REG:
			write_b(0xc1, GEN_MODRM(MOD_R, 4, dst.reg), (uint8_t)src.imm);
			break;
		case ARIFM_SHR_IMM_TO_REG:
			write_b(0xc1, GEN_MODRM(MOD_R, 5, dst.reg), (uint8_t)src.imm);
			break;
		case ARIFM_IMUL_REG_TO_REG:
			write_b(0x0f, 0xaf, GEN_MODRM(MOD_R, dst.reg, src.reg));
			break;
		case ARIFM_IDIV_REG:
			write_b(0xf7, GEN_MODRM(MOD_R, 7, src.reg));
			break;

		default:	fprintf(stderr, "arifm type out of range\n");
	}
}

void emit_lea(const mod_t mod, const operand_t dst, const operand_t base, const int32_t disp)
{
	write_b(REX_W, 0x8d, GEN_MODRM(mod, dst.reg, base.reg));

	if(mod == MOD_M_DISP)	write_d((uint32_t)disp);
}

#pragma GCC pop_options

