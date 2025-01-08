#ifndef WIN32
#include <vmstdlib.h>
#include <vmio.h>
//#include <thread.h>
#include <stdio.h>

void _exit(int c) {
	//vm_exit_app();
	//thread_end();
}
void abort() {
	_exit(0);
}

void __assert_func(const char* file, int line, const char* func, const char* msg) {
	_exit(0);
}

int __wrap_sprintf(char* buffer, const char* format, ...) {
	va_list aptr;
	int ret;

	va_start(aptr, format);
	ret = vsprintf(buffer, format, aptr);
	va_end(aptr);

	return(ret);
}
#endif // !WIN32