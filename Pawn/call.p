#include <a_samp>

stock simple_call() {
    new rand = random ( 100 );
    return rand;
}

main() {
    printf("simple_rand() = %d", simple_call());
}
