/* All useg registers are 64-bit registers */

#define REX_W	0x48	/* prefix for use 64-bit operand size */

/* ------------------------- MOV --------------------------- */
/* mov reg, reg */
#define GEN_MOV_RR(dst, src)		emit_mov(MOV_REG_TO_MEM, MOD_R, src, (const operand_t){ .reg = dst }, 0)

/* mov reg, [base + disp32] */
#define GEN_MOV_RM(dst, src, disp)	emit_mov(MOV_MEM_TO_REG, MOD_M_DISP, dst, (const operand_t){ .reg = src }, disp)

/* mov [base + disp32], reg */
#define GEN_MOV_MR(dst, src, disp)	emit_mov(MOV_REG_TO_MEM, MOD_M_DISP, src, (const operand_t){ .reg = dst }, disp)

/* mov reg, imm64 */
#define GEN_MOV_RI(dst, i)		emit_mov(MOV_IMM_TO_REG, MOD_R, dst, (const operand_t){ .imm = i }, 0)

/* lea reg, [base + disp32] */
#define GEN_LEA(r, b, disp)		emit_lea(MOD_M_DISP, (const operand_t){ .reg = r }, (const operand_t){ .reg = b }, disp)

/* ------------------------- PUSH --------------------------- */

/* push [base + disp32] */
#define GEN_PUSH_M(base, disp)	emit_push(PUSH_MEM, MOD_M_DISP, (const operand_t){ .reg = base }, disp)

/* push reg */
#define GEN_PUSH_R(r)		emit_push(PUSH_REG, MOD_R, (const operand_t){ .reg = r }, 0)

/* push imm32 */
#define GEN_PUSH_I(i)		emit_push(PUSH_IMM, MOD_R, (const operand_t){ .imm = i }, 0)

/* ------------------------- POP --------------------------- */
/* pop [base + disp32] */
#define GEN_POP_M(base, disp)	emit_pop(POP_MEM, MOD_M_DISP, (const operand_t){ .reg = base }, disp)

/* pop reg */
#define GEN_POP_R(r)		emit_pop(POP_REG, MOD_R, (const operand_t){ .reg = r }, 0)

/* ------------ GEN_ENTER, GEN_LEAVE, GEN_RET, GEN_SYSCALL------------------ */
/* enter shift16, 0 */
// TODO: add prefix to define
#define GEN_ENTER(shift)	emit_enter(shift)

/* leave */
#define GEN_LEAVE		emit_leave()

/* ret */
#define GEN_RET			emit_ret()

/* syscall */
#define GEN_SYSCALL		emit_syscall()

/* ------------------------- BRANCH --------------------------- */
/* jmp reg */
#define GEN_JMP_R(r)		emit_abs_branch(B_JMP_ABS, MOD_R, (const operand_t){ .reg = r })

/* call reg */
#define GEN_CALL_R(r)		emit_abs_branch(B_CALL_ABS, MOD_R, (const operand_t){ .reg = r })

/* declare label, for example: GEN_LBL("label_name%d", label_index) */
#define GEN_LBL(l, ...)		emit_lbl(l, ##__VA_ARGS__)

/* call label */
#define GEN_CALL_L(l, ...)	emit_rel_branch(B_CALL_REL, l, ##__VA_ARGS__)

/* jmp label */
#define GEN_JMP_L(l, ...)	emit_rel_branch(B_JMP_REL, l, ##__VA_ARGS__)

/* other jumps */
#define GEN_JE(l, ...)		emit_rel_branch(B_JE, l, ##__VA_ARGS__)
#define GEN_JNE(l, ...)		emit_rel_branch(B_JNE, l, ##__VA_ARGS__)
#define GEN_JG(l, ...)		emit_rel_branch(B_JG, l, ##__VA_ARGS__)
#define GEN_JL(l, ...)		emit_rel_branch(B_JL, l, ##__VA_ARGS__)
#define GEN_JGE(l, ...)		emit_rel_branch(B_JGE, l, ##__VA_ARGS__)
#define GEN_JLE(l, ...)		emit_rel_branch(B_JLE, l, ##__VA_ARGS__)

/* add reg, reg */
#define GEN_ADD_RR(dst, src)	emit_arifm(ARIFM_ADD_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })

/* add reg, imm32 */
#define GEN_ADD_RI(dst, i)	emit_arifm(ARIFM_ADD_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = i })

/* sub reg, reg */
#define GEN_SUB_RR(dst, src)	emit_arifm(ARIFM_SUB_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })

/* sub reg, imm32 */
#define GEN_SUB_RI(dst, i)	emit_arifm(ARIFM_SUB_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = i })

/* imul reg, reg */
#define GEN_IMUL_RR(dst, src)	emit_arifm(ARIFM_IMUL_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })

/* cmp reg, reg */
#define GEN_CMP_RR(dst, src)	emit_arifm(ARIFM_CMP_REG_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .reg = src })

/* cmp reg, imm32 */
#define GEN_CMP_RI(r, i)	emit_arifm(ARIFM_CMP_REG_TO_IMM, (const operand_t){ .reg = r }, (const operand_t){ .imm = i })

/* idiv reg */
#define GEN_IDIV_R(src)		emit_arifm(ARIFM_IDIV_REG, (const operand_t){}, (const operand_t){ .reg = src })

/* shl reg, imm8 */
#define GEN_SHL_RI(dst, c)	emit_arifm(ARIFM_SHL_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = c })

/* shr reg, imm8 */
#define GEN_SHR_RI(dst, c)	emit_arifm(ARIFM_SHR_IMM_TO_REG, (const operand_t){ .reg = dst }, (const operand_t){ .imm = c })

#define write_nasm(code, ...)	\
	do { fprintf(NASM, code, ##__VA_ARGS__); } while(0)

#define write_elf(type, ...)								\
	do										\
	{										\
		fwrite(&(const type[]){ __VA_ARGS__ },					\
				sizeof(type),						\
				sizeof((const type[]){ __VA_ARGS__ })/sizeof(type),	\
				emitter_get_elf());					\
	} while(0)

#define write_b(...)	write_elf(uint8_t, __VA_ARGS__)
#define write_w(...)	write_elf(uint16_t, __VA_ARGS__)
#define write_d(...)	write_elf(uint32_t, __VA_ARGS__)
#define write_q(...)	write_elf(uint64_t, __VA_ARGS__)

