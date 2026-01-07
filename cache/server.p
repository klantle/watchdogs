native printf(const format[], {Float,_}:...);

#if defined LOCALHOST
  #define MAC "localhost"
#else
  #define MAC "non-localhost"
#endif

main() {
  printf("-----------------------");
  printf("Hello, World!");
  printf("Running in %s", MAC);
  printf("My name is %s", "John");
  printf("My age is %d", 26);
  printf("-----------------------");
}
