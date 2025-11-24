#include <a_samp>

stock foo() {
    return random(2) + 1;
}

main() {
    new ret = foo();
    if (foo == 1) {}
    else if (foo == 2) {}

    new name[] = "John Sean";
    if (strfind(name, "Sean"))
        printf("Sean!");
    if (strcmp(name, "John Sean") == 0)
        printf("John Sean!");
}
