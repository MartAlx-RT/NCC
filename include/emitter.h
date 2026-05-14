#include <fcntl.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

/* enums */

typedef enum mod_t
{
	MOD_R		= 0b11,
	MOD_M		= 0b00,
	MOD_M_DISP8	= 0b01,
	MOD_M_DISP32	= 0b10
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

typedef enum branch_t
{
	JMP_REL8	= 0xeb,
	JMP_REL16	= 0xe9,
	JMP_REL32	= 0xe9,
	JMP_ABS		= 0xff,

	CALL_REL16	= 0xe8,
	CALL_REL32	= 0xe8,
	CALL_ABS	= 0xff
} branch_t;

typedef enum fixup_type_t
{
	FIX_JMP,
	FIX_CALL
} fixup_type_t;

/* unions & structs */

typedef union operand_t
{
	uint64_t imm;
	reg_t reg;
} operand_t;

typedef struct ref_t
{
	ssize_t pos;
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
	ssize_t pos;
	const char *name;
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

void emitter_init(FILE *nasm, FILE *elf);
void emitter_deinit(void);
size_t emitter_get_elf_pos(void);
void emitter_fixup_add_ref(const ref_t ref);
void emitter_fixup_add_lbl(const lbl_t lbl);
void emitter_fixup(void);

void emit_mov(const mov_t type, const mod_t mod, const reg_t reg, const operand_t op, const uint32_t disp);
void emit_push(const push_t type, const operand_t op);
void emit_enter(uint16_t shift);
void emit_leave(void);
void emit_ret(void);
void emit_syscall(void);
void emit_branch(const branch_t type, const mod_t mod, const operand_t op);
void emit_abs_branch(const branch_t branch, const operand_t op);
void emit_rel_branch(const fixup_type_t type, const char *lbl);
void emit_lbl(const char *lbl);

