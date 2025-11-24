#include <a_samp>

main() {
    new k;
back:
    k = random(100) + 1;
    switch (k) {
        case 1 .. 50: {
            printf("1 - 50");
            goto back;
        }
        case 51,52,53,54,55: {
            printf("51,52,53,54,55");
            goto back;
        }
        case 56 .. 59: {
            printf("56");
            goto back;
        }
        case 100: {
            printf("100");
        }
    }
}
