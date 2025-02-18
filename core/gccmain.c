#include <stdio.h>
#include "vmsys.h"
#include "vmchset.h"
#include "vmlog.h"
#include "vmio.h"
#include "vmstdlib.h"

extern unsigned int __init_array_start;
extern unsigned int __init_array_end;

typedef VMINT(*vm_get_sym_entry_t)(char* symbol);
vm_get_sym_entry_t vm_get_sym_entry;
typedef void (**__init_array) (void);

int __g_errno = 0;

void* malloc(size_t size)
{
	return vm_malloc(size);
}

void* calloc(size_t number, size_t size)
{
	return vm_calloc(size * number);
}

void* realloc(void* ptr, size_t newsize) {
	return vm_realloc(ptr, newsize);
}

void free(void* ptr)
{
	vm_free(ptr);
}

void* _malloc_r(struct _reent* unused, size_t size)
{
	(void)unused;
	void* ret = malloc(size);
	return ret;
}

void* _calloc_r(struct _reent* unused, size_t count, size_t size)
{
	(void)unused;
	void* ret = calloc(count, size);
	return ret;
}

void* _realloc_r(struct _reent* unused, void* ptr, size_t newsize)
{
	(void)unused;
	void* ret = realloc(ptr, newsize);
	return ret;
}
void _free_r(struct _reent* unused, void* ptr)
{
	(void)unused;
	free(ptr);
}

void trace_on(){
	typedef void (*trace_on_t)();
	static trace_on_t trace_on_p = (trace_on_t)0xFFFFFFFF;
	if(trace_on_p == (trace_on_t)0xFFFFFFFF)
		trace_on_p = (trace_on_t)vm_get_sym_entry("trace_on");
	if(trace_on_p != 0 && trace_on_p != (trace_on_t)0xFFFFFFFF)
		trace_on_p();
	return;
}

void gcc_entry(unsigned int entry, unsigned int init_array_start, unsigned int count)
{
	unsigned int i;
	__init_array ptr;
	vm_get_sym_entry = (vm_get_sym_entry_t)entry;

	if (init_array_start == 0)
		init_array_start = (unsigned int)&__init_array_start,
		count = ((unsigned int)&__init_array_end - (unsigned int)&__init_array_start) / 4;

	ptr = (__init_array)init_array_start;
	for (i = 1; i < count; i++)
	{
		ptr[i]();
	}
	vm_main();
}
