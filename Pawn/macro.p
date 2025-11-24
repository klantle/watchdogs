#include <a_samp>

#define MY_STR::
#define MY_INT::
#define MY_PTR::
#define MY_VOID::  stock
#define MY_FLOAT:: Float:
#define MY_BOOL::  bool:
#define int        new
#define string     new
#define ptr        new
#define my_if      if
#define my_else    else
#define my_elif    else if

#define MMM_STRING  "name"
#define MMM_INTEGAR 100

main() {
    new MY_STR::name[26] = "John";
    new MY_INT::age      = 26;
    new MY_PTR::job[]    = "none";
    new MY_FLOAT::radius = 100.0;
    new MY_BOOL::status  = false;
    int my_age           = 26;
    string my_names[26]  = "";
    ptr my_namess[]      = "";
    printf("%s", MMM_STRING);
    printf("%d", MMM_INTEGAR);
    new test = random(2) + 1;
    my_if (test == 1) {}
    my_elif (test == 2) {}
    my_else {}
}
