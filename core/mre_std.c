#include <stdio.h>
#include <vmio.h>
#include <vmchset.h>
#include <vmstdlib.h>

#include "console.h"

FILE* mre_fopen(char const* filename, char const* f_mode) {
	if (filename[0] == '.' && filename[1] == '\\')
		filename += 2;

	VMCHAR wstr[260];
	vm_ascii_to_ucs2(wstr, 260 * 2, filename);

	int mode = 0;
	if (f_mode[0] == 'r')
		mode = MODE_READ;
	if (f_mode[0] == 'w')
		mode = MODE_CREATE_ALWAYS_WRITE;

	int h = vm_file_open(wstr, mode, 1);
	if (h < 0)
		return 0;
	return (FILE*)h;
}

size_t mre_fread(void* buf, size_t element_size, size_t element_count, FILE* stream) {
	VMUINT nread = 0;
	return vm_file_read((int)stream, buf, element_size * element_count, &nread);
}

size_t mre_fwrite(void* buf, size_t element_size, size_t element_count, FILE* stream) {
	VMUINT written = 0;
	return vm_file_write((int)stream, buf, element_size * element_count, &written);
}

long mre_ftell(FILE* stream) {
	return vm_file_tell((int)stream);
}

int mre_fseek(FILE* stream, long offset, int origin) {
	return vm_file_seek((int)stream, offset, origin + 1);
}

int mre_fclose(FILE* stream) {
	vm_file_close((int)stream);
	return 0;
}

int mre_fflush(FILE* stream) {
	return 0;
}

int mre_mkdir(char const* path) {
	VMCHAR wstr[260];
	vm_ascii_to_ucs2(wstr, 260 * 2, path);

	return 0;//vm_file_mkdir(wstr);
}

int mre_remove(char const* filename) {
	VMCHAR wstr[260];
	vm_ascii_to_ucs2(wstr, 260 * 2, filename);

	return vm_file_delete(wstr);
}

int mre_rename(char const* oldfilename, char const* newfilename) {
	VMCHAR wstr1[260], wstr2[260];
	vm_ascii_to_ucs2(wstr1, 260 * 2, oldfilename);
	vm_ascii_to_ucs2(wstr2, 260 * 2, newfilename);
	return vm_file_rename(wstr1, wstr2);
}


int mre_putchar(int c) {
	console_put_char(c);
}

int mre_puts(char const* str) {
	console_put_str(str);
}

static char buf[1024];

int mre_printf(char const* const format, ...) {
	va_list aptr;

	va_start(aptr, format);
	int ret = vsprintf(buf, format, aptr);
	va_end(aptr);

	console_put_str(buf);
	return ret;
}

int mre_fprintf(int un, char const* const format, ...) {
	va_list aptr;

	va_start(aptr, format);
	int ret = vsprintf(buf, format, aptr);
	va_end(aptr);

	console_put_str(buf);
	return ret;
}

int mre_vfprintf(int un, char const* const format, va_list argptr) {
	int ret = vsprintf(buf, format, argptr);

	console_put_str(buf);
	return ret;
}