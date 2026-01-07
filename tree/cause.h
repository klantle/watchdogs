#ifndef CAUSE
#define CAUSE

typedef struct {
        char *cs_t;
        char *cs_i;
} causeExplanation;

void cause_compiler_expl(const char *log_file,const char *dogoutput,int debug);    

#endif
