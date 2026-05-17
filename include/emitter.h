#ifndef EMITTER_H
#define EMITTER_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/user.h>

#define ELF_ENTRY_VA	0x400000
#define ELF_START_OFF	PAGE_SIZE

/* enums */

typedef enum mod_t
{
	MOD_R		= 0b11,
	MOD_M		= 0b00,
	MOD_M_DISP	= 0b10
} mod_t;

typedef enum reg_t
{
	RAX	= 0,
	RCX	= 1,
	RDX	= 2,
	RBX	= 3,
	RSP	= 4,
	RBP	= 5,
	RSI	= 6,
	RDI	= 7
} reg_t;

typedef enum mov_t
{
	MOV_REG_TO_MEM	= 0x89,
	MOV_MEM_TO_REG	= 0x8b,
	MOV_IMM_TO_REG	= 0xb8,
	MOV_IMM_TO_MEM	= 0xc7
} mov_t;

typedef enum push_t
{
	PUSH_REG	= 0x50,
	PUSH_MEM	= 0xff,
	PUSH_IMM	= 0x68
} push_t;

typedef enum pop_t
{
	POP_REG		= 0x58,
	POP_MEM		= 0x8f
} pop_t;

typedef enum branch_t
{
	B_JMP_REL	= 0xe9,
	B_CALL_REL	= 0xe8,

	B_JE		= 0x84,
	B_JNE		= 0x85,
	B_JG		= 0x8f,
	B_JL		= 0x8c,

	B_JMP_ABS	= 0xff,
	B_CALL_ABS	= 0xff
} branch_t;

typedef enum arifm_t
{
	ARIFM_ADD_REG_TO_REG	= 0x3,
	ARIFM_SUB_REG_TO_REG	= 0x2b,
	ARIFM_CMP_REG_TO_REG	= 0x3b,
	ARIFM_CMP_REG_TO_IMM	= 0x81,

	/* not typical instruction */
	ARIFM_ADD_IMM_TO_REG,
	ARIFM_SUB_IMM_TO_REG,
	ARIFM_SHL_IMM_TO_REG,
	ARIFM_SHR_IMM_TO_REG,
	ARIFM_IMUL_REG_TO_REG,
	ARIFM_IDIV_REG
} arifm_t;

/* unions & structs */

typedef union operand_t
{
	int64_t imm;
	reg_t reg;
} operand_t;

typedef struct ref_t
{
	ssize_t pos;
	branch_t type;
	char *name;
} ref_t;

typedef struct refs_t
{
	ref_t *refs;
	size_t size, cap;
} refs_t;

typedef struct lbl_t
{
	ssize_t pos;
	char *name;
} lbl_t;

typedef struct lbls_t
{
	lbl_t *lbls;
	size_t size, cap;
} lbls_t;

typedef struct fixups_t
{
	lbls_t lbls;
	refs_t refs;
} fixups_t;

/* common functions */ 
void emitter_init(FILE *elf, FILE *nasm);
void emitter_deinit(void);
void emitter_free_names(void);
ssize_t emitter_get_elf_pos(void);
FILE *emitter_get_elf(void);
void emitter_fixup_add_ref(const ref_t ref);
void emitter_fixup(void);
char *emitter_make_msg(const char *fmt, va_list args);
void emitter_fixup_add_lbl(const lbl_t lbl);

/* emit instruction functions */
void emit_mov(const mov_t type, const mod_t mod, const reg_t reg, const operand_t op, const int32_t disp);
void emit_enter(uint16_t shift);
void emit_leave(void);
void emit_ret(void);
void emit_syscall(void);
void emit_abs_branch(const branch_t branch, const operand_t op);
void emit_rel_branch(const branch_t type, const char *fmt, ...);
void emit_lbl(const char *fmt, ...);
void emit_push(const push_t type, mod_t mod, const operand_t op, const int32_t disp);
void emit_pop(const pop_t type, mod_t mod, const operand_t op, const int32_t disp);
void emit_arifm(const arifm_t type, operand_t dst, operand_t src);
void emit_lea(const mod_t mod, const operand_t dst, const operand_t base, const int32_t disp);

#endif /* EMITTER_H */

