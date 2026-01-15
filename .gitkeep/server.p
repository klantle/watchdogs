#if ! defined printf
native
	printf(const format[],
	     {Float,_}:...);
#endif

main() {
   printf "Hello, World!"
}
