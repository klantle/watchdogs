#include <a_samp>

#define my_funcs::%0(%1) \
        forward %0(%1); \
        public %0(%1)

my_funcs::foo() {
    printf("foo!");
    return 1;
}

main() {
    foo();
}
