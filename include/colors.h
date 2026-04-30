#ifndef COLORS_H
#define COLORS_H


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

#endif
