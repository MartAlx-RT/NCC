#define ROOT				(const node_data_t){.type = TP_ROOT, .val.num = 0}
#define OP_SEQ				(const node_data_t){.type = TP_OP_SEQ, .val.num = 0}
#define PARAM				(const node_data_t){.type = TP_PARAM, .val.num = 0}
#define DEREF				(const node_data_t){.type = TP_DEREF, .val.num = 0}

#define OPN_BRC				(const node_data_t){.type = TP_SYMB, .val.symb = SYM_OPN_BRC}
#define CLS_BRC 			(const node_data_t){.type = TP_SYMB, .val.symb = SYM_CLS_BRC}
#define OPN_PAR 			(const node_data_t){.type = TP_SYMB, .val.symb = SYM_OPN_PAR}
#define CLS_PAR 			(const node_data_t){.type = TP_SYMB, .val.symb = SYM_CLS_PAR}
#define SEMICOLON 			(const node_data_t){.type = TP_SYMB, .val.symb = SYM_SEMICOL}
#define COMMA 				(const node_data_t){.type = TP_SYMB, .val.symb = SYM_COMMA}
#define OPN_BRK				(const node_data_t){.type = TP_SYMB, .val.symb = SYM_OPN_BRK}
#define CLS_BRK				(const node_data_t){.type = TP_SYMB, .val.symb = SYM_CLS_BRK}
#define EQ 					(const node_data_t){.type = TP_OP, .val.op = OP_EQ}
#define ASSIGN 				(const node_data_t){.type = TP_OP, .val.op = OP_ASSIGN}
#define ADD 				(const node_data_t){.type = TP_OP, .val.op = OP_ADD}
#define SUB 				(const node_data_t){.type = TP_OP, .val.op = OP_SUB}
#define GREATER 			(const node_data_t){.type = TP_OP, .val.op = OP_GREATER}
#define LESS 				(const node_data_t){.type = TP_OP, .val.op = OP_LESS}
#define MUL 				(const node_data_t){.type = TP_OP, .val.op = OP_MUL}
#define DIV 				(const node_data_t){.type = TP_OP, .val.op = OP_DIV}
#define OR 					(const node_data_t){.type = TP_OP, .val.op = OP_OR}
#define AND 				(const node_data_t){.type = TP_OP, .val.op = OP_AND}
#define FUNC				(const node_data_t){.type = TP_KWORD, .val.kword = KW_FUNC}
#define IF 					(const node_data_t){.type = TP_KWORD, .val.kword = KW_IF}
#define ELSE 				(const node_data_t){.type = TP_KWORD, .val.kword = KW_ELSE}
#define WHILE 				(const node_data_t){.type = TP_KWORD, .val.kword = KW_WHILE}
#define FOR 				(const node_data_t){.type = TP_KWORD, .val.kword = KW_FOR}
#define ASM 				(const node_data_t){.type = TP_KWORD, .val.kword = KW_ASM}
#define RETURN				(const node_data_t){.type = TP_KWORD, .val.kword = KW_RETURN}
#define PASS				(const node_data_t){.type = TP_KWORD, .val.kword = KW_PASS}
#define BREAK				(const node_data_t){.type = TP_KWORD, .val.kword = KW_BREAK}
#define CONTINUE			(const node_data_t){.type = TP_KWORD, .val.kword = KW_CONTINUE}

#define FUNC_DECL(f_name)	(const node_data_t){.type = TP_DECL_FUNC, .val.name = f_name}
#define FUNC_CALL(f_name)	(const node_data_t){.type = TP_CALL_FUNC, .val.name = f_name}
#define VAR(var_id)			(const node_data_t){.type = TP_VAR, .val.id = var_id}
#define TAKEADDR(var_id)	(const node_data_t){.type = TP_TAKEADDR, .val.id = var_id}
#define NUM(n)				(const node_data_t){.type = TP_NUM, .val.num = n}
#define CHILD_EXISTS(ch)	(ch && ch->node)
#define IS_BINNODE(tr)		(CHILD_EXISTS(tr->child) && CHILD_EXISTS(tr->child->next) && !CHILD_EXISTS(tr->child->next->next))
#define LEFT(tr)			tr->child->node
#define RIGHT(tr)			tr->child->next->node

#define LEAVE_IF_ERR    \
	if (COMPILE_STATUS) \
		return;

#define err_exit_msg(msg)   \
	{                       \
		print_err_msg(msg); \
		COMPILE_STATUS = 1; \
		return;             \
	}

#define write_ntc(m, l)                                                                                    \
	do                                                                                                     \
	{                                                                                                      \
		if (ALERTS.n_alert < N_ALERT_LIMIT)                                                                \
			ALERTS.alert[ALERTS.n_alert++] = (const alert_t){.type = AL_NOTICE, .msg = m "\n", .line = l}; \
	} while (0)

#define write_wrg(m, l)                                                                                     \
	do                                                                                                      \
	{                                                                                                       \
		if (ALERTS.n_alert < N_ALERT_LIMIT)                                                                 \
			ALERTS.alert[ALERTS.n_alert++] = (const alert_t){.type = AL_WARNING, .msg = m "\n", .line = l}; \
	} while (0)

#define write_err(m, l)                                                                                   \
	do                                                                                                    \
	{                                                                                                     \
		if (ALERTS.n_alert < N_ALERT_LIMIT)                                                               \
			ALERTS.alert[ALERTS.n_alert++] = (const alert_t){.type = AL_ERROR, .msg = m "\n", .line = l}; \
	} while (0)

#define print_asm(fmt, ...)	fprintf(ASM_OUT, fmt, ##__VA_ARGS__)

#define IS_(macro, data) ((macro).type == (data).type && !memcmp(&((macro).val), &((data).val), sizeof(node_val_t)))


