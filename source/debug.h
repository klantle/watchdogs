#ifndef DEBUG_H
#define DEBUG_H

void _unit_debugger(int debug_hard,
            const char *function,
            const char *file, int line);
#define unit_debugging(wx) \
    _unit_debugger(wx, __func__, __FILE__, __LINE__)

void _minimal_debugger(const char *function,
            const char *file, int line);
#define minimal_debugging() \
    _minimal_debugger(__func__, __FILE__, __LINE__)

#endif
