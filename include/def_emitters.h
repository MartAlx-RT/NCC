#define write_asm(fmt, ...)	fprintf(ASM_OUT, fmt, ##__VA_ARGS__)

#define MOV_RR(dst, src)	emit_mov(MOV_REG_TO_MEM, MOD_R, src, (const operand_t){ .reg = dst }, 0)
#define MOV_RM(dst, src, disp)	emit_mov(MOV_MEM_TO_REG, MOD_M_DISP, dst, (const operand_t){ .reg = src }, disp)
#define MOV_MR(dst, src, disp)	emit_mov(MOV_REG_TO_MEM, MOD_M_DISP, src, (const operand_t){ .reg = dst }, disp)
#define MOV_RI(dst, i)		emit_mov(MOV_IMM_TO_REG, MOD_R, dst, (const operand_t){ .imm = i }, 0)

#define PUSH_M(base, disp)	emit_push(PUSH_MEM, MOD_M_DISP, (const operand_t){ .reg = base }, disp)
#define PUSH_R(r)		emit_push(PUSH_REG, MOD_R, (const operand_t){ .reg = r }, 0)
#define PUSH_I(i)		emit_push(PUSH_IMM, MOD_R, (const operand_t){ .imm = i }, 0)

#define POP_M(base)		emit_pop(POP_MEM, MOD_M_DISP, (const operand_t){ .reg = base }, disp)
#define POP_R(r)		emit_pop(POP_REG, MOD_R, (const operand_t){ .reg = r }, 0)

#define ENTER(shift)		emit_enter(shift)
#define LEAVE			emit_leave()
#define RET			emit_ret()
#define SYSCALL			emit_syscall()

#define JMP_R(r)		emit_abs_branch(B_JMP_ABS, MOD_R, (const operand_t){ .reg = r })
#define CALL_R(r)		emit_abs_branch(B_CALL_ABS, MOD_R, (const operand_t){ .reg = r })
#define JMP_L(l)		emit_rel_branch(B_JMP_REL, l)
#define CALL_L(l)		emit_rel_branch(B_CALL_REL, l)
#define LBL(l)			emit_lbl(l)

#define MSG(s, ...)		emitter_make_msg(s, ##__VA_ARGS__)

#define JE(l)			emit_rel_branch(B_JE, l)
#define JNE(l)			emit_rel_branch(B_JNE, l)
#define JG(l)			emit_rel_branch(B_JG, l)
#define JL(l)			emit_rel_branch(B_JL, l)

/* arifmetics
 * void emit_arifm(const arifm_t type, operand_t dst, operand_t src)
*/
#define ADD_RR(dst, src)	emit_arifm(ARIFM_ADD_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })
#define ADD_RI(dst, i)		emit_arifm(ARIFM_ADD_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = i })
#define SUB_RR(dst, src)	emit_arifm(ARIFM_SUB_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })
#define SUB_RI(dst, i)		emit_arifm(ARIFM_SUB_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = i })
#define IMUL_RR(dst, src)	emit_arifm(ARIFM_IMUL_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })
#define CMP_RR(dst, src)	emit_arifm(ARIFM_CMP_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })
#define IDIV_R(dst, src)	emit_arifm(ARIFM_IDIV_REG_TO_REG, 0, (const operand_t){ .reg = src })
#define SHL_RI(dst, c)		emit_arifm(ARIFM_SHL_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = c })
#define SHR_RI(dst, c)		emit_arifm(ARIFM_SHR_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = c })

#define REX_W	0x48

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

