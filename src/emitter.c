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

#define MODRM(mod, r, rm)	((uint8_t)((mod<<6)|(r<<3)|(rm)))

#define DEF_REG(name)	[name] = #name
static const char *REG_NAMES[] =
{
	DEF_REG(RAX),
	DEF_REG(RCX),
	DEF_REG(RDX),
	DEF_REG(RBX),
	DEF_REG(RSP),
	DEF_REG(RBP),
	DEF_REG(RSI),
	DEF_REG(RDI)
};
#undef DEF_REG


static FILE *NASM = NULL;
static FILE *ELF = NULL;
static fixups_t FIXUPS = {};

/* common functions */ 

void emitter_init(FILE *elf, FILE *nasm)
{
	assert(elf);	assert(nasm);
	ELF = elf;	NASM = nasm;

	const Elf64_Ehdr elf_header =
	{
		.e_ident =
		{
			[EI_MAG0] = ELFMAG0,
			[EI_MAG1] = ELFMAG1,
			[EI_MAG2] = ELFMAG2,
			[EI_MAG3] = ELFMAG3,

			[EI_CLASS] = ELFCLASS64,
			[EI_DATA] = ELFDATA2LSB,
			[EI_VERSION] = EV_CURRENT,
			[EI_OSABI] = ELFOSABI_SYSV,
			[EI_ABIVERSION] = 0,
			[EI_PAD] = 0
		},

		.e_flags = 0,
		.e_type = ET_EXEC,
		.e_machine = EM_X86_64,
		.e_version = EV_CURRENT,

		.e_entry = ELF_ENTRY_VA,
		.e_phoff = sizeof(Elf64_Ehdr),		/* prog hdr off */
		.e_shoff = 0,				/* section hdr tbl off */

		.e_ehsize = sizeof(Elf64_Ehdr),
		.e_phentsize = sizeof(Elf64_Phdr),
		.e_shentsize = sizeof(Elf64_Shdr),

		.e_phnum = 1,		/* number of entries */
		.e_shnum = 0,		/* entries in sections hdrs tbl */
		.e_shstrndx = SHN_UNDEF	/* section hdr tbl idx of the entry associated with the section name str tbl */
	};

	const Elf64_Phdr program_header =
	{
		.p_type = PT_LOAD,
		.p_offset = ELF_START_OFF,	/* file pos off */
		.p_vaddr = ELF_ENTRY_VA,
		.p_paddr = 0xDED,

		.p_filesz = 0,			/* needed fixup */
		.p_memsz = 0,			/* needed fixup */

		.p_flags = PF_R | PF_X,
		.p_align = PAGE_SIZE
	};

	fwrite(&elf_header, sizeof(elf_header), 1, ELF);
	fwrite(&program_header, sizeof(program_header), 1, ELF);

	fseek(emitter_get_elf(), ELF_START_OFF, SEEK_SET);
}

void emitter_deinit(void)
{
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

void emitter_free_names(void)
{
	for(size_t i = 0; i < FIXUPS.lbls.size; i++)
		free(FIXUPS.lbls.lbls[i].name);

	for(size_t i = 0; i < FIXUPS.refs.size; i++)
		free(FIXUPS.refs.refs[i].name);
}

ssize_t emitter_get_elf_pos(void)
{
	return ftell(ELF);
}

FILE *emitter_get_elf(void)
{
	return ELF;
}

FILE *emitter_get_nasm(void)
{
	return NASM;
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

		if(lbl == NULL)	fprintf(stderr, "unknown label: `%s`\n", ref->name);
		else
		{
			ssize_t distance = lbl->pos - ref->pos;
			size_t abs_distance = (size_t) ((distance > 0)? distance : -distance);
			if(abs_distance >= INT32_MAX)
				fprintf(stderr, "`%s`: distance is too large\n", lbl->name);

			if(ref->type == B_CALL_REL || ref->type == B_JMP_REL)
				elf_seek(ref->pos - 5);
			else
			{
				elf_seek(ref->pos - 6);	write_b(0x0f);
			}
			write_b(ref->type);	write_d((uint32_t)distance);
		}
	}

	fseek(emitter_get_elf(), current_pos, SEEK_SET);
}

/* emit instruction functions */

void emit_mov(const mov_t type, const mod_t mod, const reg_t reg, const operand_t op, const int32_t disp)
{
	switch(type)
	{
		case MOV_IMM_TO_REG:
					write_nasm("\tmov\t%s, %ld\n", REG_NAMES[reg], op.imm);
					break;
		case MOV_REG_TO_MEM:
					write_nasm("\tmov\tqword [%s], %s\n",  REG_NAMES[reg], REG_NAMES[op.reg]);
					break;
		case MOV_MEM_TO_REG:
					write_nasm("\tmov\t%s, qword [%s]\n", REG_NAMES[reg], REG_NAMES[op.reg]);
					break;

		case MOV_IMM_TO_MEM:
		default:		fprintf(stderr, "mov type out of range\n");
	}

	write_b(REX_W);
	switch(type)
	{
		case MOV_IMM_TO_REG:
					write_b((uint8_t)(type + reg));
					write_q((uint64_t)op.imm);
					break;

		case MOV_REG_TO_MEM:
		case MOV_MEM_TO_REG:
					write_b(type, MODRM(mod, reg, op.reg));
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
		case PUSH_REG:
			write_nasm("\tpush\t%s\n", REG_NAMES[op.reg]);
			write_b((uint8_t)(PUSH_REG + op.reg));
			break;
		case PUSH_IMM:
			write_nasm("\tpush\t%ld\n", op.imm);
			write_b(PUSH_IMM);	write_d((uint32_t)op.imm);
			break;
		case PUSH_MEM:
			write_nasm("\tpush\tqword [%s%+d]\n", REG_NAMES[op.reg], disp);
			write_b(PUSH_MEM, MODRM(mod, 6, op.reg));
			if(mod == MOD_M_DISP)	write_d((uint32_t)disp);
			break;

		default:	fprintf(stderr, "push type out of range\n");
	}
}

void emit_pop(const pop_t type, mod_t mod, const operand_t op, const int32_t disp)
{
	switch(type)
	{
		case POP_REG:
			write_nasm("\tpop\t%s\n", REG_NAMES[op.reg]);
			write_b((uint8_t)(POP_REG + op.reg));
			break;

		case POP_MEM:
			write_nasm("\tpop\tqword [%s%+d]\n", REG_NAMES[op.reg], disp);
			write_b(POP_MEM, MODRM(mod, 0, op.reg));
			if(mod == MOD_M_DISP)	write_d((uint32_t)disp);
			break;

		default:	fprintf(stderr, "pop type out of range\n");
	}
}

void emit_abs_branch(const branch_t type, const operand_t op)
{
	if(type == B_JMP_ABS)	write_nasm("\tjmp\t%s\n", REG_NAMES[op.reg]);
	else			write_nasm("\tcall\t%s\n", REG_NAMES[op.reg]);

	write_b((uint8_t)type, MODRM(6, 4, op.reg));
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

	const char *nasm_fmt = NULL;
	switch(type)
	{
		case B_JMP_REL:		nasm_fmt = "\tjmp\t%s\n";	break;
		case B_CALL_REL:	nasm_fmt = "\tcall\t%s\n";	break;
		case B_JE:		nasm_fmt = "\tje\t%s\n";	break;
		case B_JNE:		nasm_fmt = "\tjne\t%s\n";	break;
		case B_JG:		nasm_fmt = "\tjg\t%s\n";	break;
		case B_JL:		nasm_fmt = "\tjl\t%s\n";	break;

		case B_JMP_ABS:
		default:
					fprintf(stderr, "jmp type out of range\n");
	}
	write_nasm(nasm_fmt, ref.name);

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

	write_nasm("%s:\n", lbl.name);

	emitter_fixup_add_lbl(lbl);
}

void emit_enter(const uint16_t shift)
{
	write_nasm("\tenter\t%d, 0\n", shift);
	write_b(0xc8);
	write_w(shift);	write_b(0x00);
}

void emit_leave(void)
{
	write_nasm("\tleave\n");
	write_b(0xc9);
}

void emit_ret(void)
{
	write_nasm("\tret\n");
	write_b(0xc3);
}

void emit_syscall(void)
{
	write_nasm("\tsyscall\n");
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
	switch(type)
	{
		case ARIFM_CMP_REG_TO_REG:
			write_nasm("\tcmp\t%s, %s\n", REG_NAMES[dst.reg], REG_NAMES[src.reg]);
			break;
		case ARIFM_SUB_REG_TO_REG:
			write_nasm("\tsub\t%s, %s\n", REG_NAMES[dst.reg], REG_NAMES[src.reg]);
			break;
		case ARIFM_ADD_REG_TO_REG:
			write_nasm("\tadd\t%s, %s\n", REG_NAMES[dst.reg], REG_NAMES[src.reg]);
			break;
		case ARIFM_CMP_REG_TO_IMM:
			write_nasm("\tcmp\t%s, %ld\n", REG_NAMES[dst.reg], src.imm);
			break;
		case ARIFM_ADD_IMM_TO_REG:
			write_nasm("\tadd\t%s, %ld\n", REG_NAMES[dst.reg], src.imm);
			break;
		case ARIFM_SUB_IMM_TO_REG:
			write_nasm("\tsub\t%s, %ld\n", REG_NAMES[dst.reg], src.imm);
			break;
		case ARIFM_SHL_IMM_TO_REG:
			write_nasm("\tshl\t%s, %ld\n", REG_NAMES[dst.reg], src.imm);
			break;
		case ARIFM_SHR_IMM_TO_REG:
			write_nasm("\tshr\t%s, %ld\n", REG_NAMES[dst.reg], src.imm);
			break;
		case ARIFM_IMUL_REG_TO_REG:
			write_nasm("\timul\t%s, %ld\n", REG_NAMES[dst.reg], src.imm);
			break;
		case ARIFM_IDIV_REG:
			write_nasm("\tdiv\t%s\n", REG_NAMES[src.reg]);
			break;

		default:	fprintf(stderr, "arifm type out of range\n");
	}

	write_b(REX_W);
	switch(type)
	{
		case ARIFM_ADD_REG_TO_REG:
		case ARIFM_SUB_REG_TO_REG:
		case ARIFM_CMP_REG_TO_REG:
			write_b((uint8_t)type, MODRM(MOD_R, dst.reg, src.reg));
			break;

		case ARIFM_CMP_REG_TO_IMM:
			write_b(ARIFM_CMP_REG_TO_IMM, MODRM(MOD_R, 7, dst.reg));
			write_d((uint32_t)src.imm);
			break;

		/* without unique opcode */
		case ARIFM_ADD_IMM_TO_REG:
			write_b(0x83, MODRM(MOD_R, 0, dst.reg), (uint8_t)src.imm);
			break;
		case ARIFM_SUB_IMM_TO_REG:
			write_b(0x83, MODRM(MOD_R, 5, dst.reg), (uint8_t)src.imm);
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

void emit_lea(const mod_t mod, const operand_t dst, const operand_t base, const int32_t disp)
{
	write_nasm("\tlea\t%s, [%s%+d]\n", REG_NAMES[dst.reg], REG_NAMES[base.reg], disp);

	write_b(REX_W, 0x8d, MODRM(mod, dst.reg, base.reg));

	if(mod == MOD_M_DISP)	write_d((uint32_t)disp);
}

#pragma GCC pop_options

