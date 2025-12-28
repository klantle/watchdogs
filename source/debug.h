#ifndef DEBUG_H
#define DEBUG_H

void __debug_main_unit_(int debug_hard,
            const char *function,
            const char *pretty_function,
            const char *file, int line);
#define __create_unit_logging(wx) \
    __debug_main_unit_(wx, __func__, __PRETTY_FUNCTION__, __FILE__, __LINE__)

void __debug_function_(const char *function,
            const char *pretty_function,
            const char *file, int line);
#define __create_logging() \
    __debug_function_(__func__, __PRETTY_FUNCTION__, __FILE__, __LINE__)

#endif