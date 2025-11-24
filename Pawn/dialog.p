#include <a_samp>

enum {
    MY_DIALOG = 0
};

main() {
    new show = ShowPlayerDialog(0,
                                MY_DIALOG,
                                DIALOG_STYLE_LIST,
                                "-",
                                "-",
                                "-",
                                "-");
    if (show) {
        printf("succes!");
    }
}
