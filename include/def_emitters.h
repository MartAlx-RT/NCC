#define write_asm(fmt, ...)	fprintf(ASM_OUT, fmt, ##__VA_ARGS__)

#define MOV_RR(dst, src)	emit_mov(MOV_REG_TO_MEM, MOD_R, src, (const operand_t){ .reg = dst }, 0)
#define MOV_RM(dst, src)	emit_mov(MOV_MEM_TO_REG, MOD_M, dst, (const operand_t){ .reg = src }, 0)
#define MOV_MR(dst, src)	emit_mov(MOV_REG_TO_MEM, MOD_M, src, (const operand_t){ .reg = dst }, 0)
#define MOV_RI(dst, i)		emit_mov(MOV_IMM_TO_REG, MOD_R, dst, (const operand_t){ .imm = i }, 0)

#define PUSH_M(base)		emit_push(PUSH_MEM, (const operand_t){ .reg = base })
#define PUSH_R(r)		emit_push(PUSH_REG, (const operand_t){ .reg = r })
#define PUSH_I(i)		emit_push(PUSH_IMM, (const operand_t){ .imm = i })

#define ENTER(shift)		emit_enter(shift)
#define LEAVE			emit_leave()
#define RET			emit_ret()
#define SYSCALL			emit_syscall()

#define JMP_R(r)		emit_branch(JMP_REL, MOD_R, (const operand_t){ .reg = r })
#define CALL_R(r)		emit_branch(CALL_REL, MOD_R, (const operand_t){ .reg = r })
#define JMP_L(l)		emit_rel_branch(FIX_JMP, l)
#define CALL_L(l)		emit_rel_branch(FIX_CALL, l)
#define LBL(l)			emit_lbl(l)

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

#define crnt_pos	ftell(ELF)
#define elf_seek(off)	fseek(ELF, off, SEEK_SET)
#define elf_seek_end()	fseek(ELF, 0, SEEK_END)

