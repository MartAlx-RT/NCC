#define _RESET_COLORS_ "\033[0m"

#define _GRAY_      "\033[90m"
#define _RED_       "\033[91m"
#define _GREEN_     "\033[92m"
#define _YELLOW_    "\033[93m"
#define _BLUE_      "\033[94m"
#define _MAGENTA_   "\033[95m"
#define _CYAN_      "\033[96m"
#define _WHITE_     "\033[97m"
#define _BOLD_		"\033[1m"

#define colorize(expr, col) \
col expr _RESET_COLORS_

#define print_err_msg(msg) \
	fprintf(stderr, colorize("%s: ", _MAGENTA_) colorize(msg, _BOLD_ _RED_) "\n", __func__)

#define print_wrong_s(s) \
	fprintf(stderr, colorize("-->", _BOLD_ _YELLOW_) colorize("%.20s", _CYAN_) colorize("...\n\n", _BOLD_ _YELLOW_), s)

#define print_wrg_msg(msg) \
	fprintf(stderr, colorize(msg, _BOLD_ _MAGENTA_) "\n")
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

