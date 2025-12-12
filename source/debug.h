#ifndef DEBUG_H
#define DEBUG_H

void __debug_main_chain_(int debug_hard,
            const char *function,
            const char *pretty_function,
            const char *file, int line);
#define __debug_main_chain(wx) \
    __debug_main_chain_(wx, __func__, __PRETTY_FUNCTION__, __FILE__, __LINE__)

void __debug_function_(const char *function,
            const char *pretty_function,
            const char *file, int line);
#define __debug_function() \
    __debug_function_(__func__, __PRETTY_FUNCTION__, __FILE__, __LINE__)

#endif