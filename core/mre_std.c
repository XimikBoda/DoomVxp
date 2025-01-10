#include <stdio.h>
#include <vmio.h>
#include <vmchset.h>
#include <vmstdlib.h>

#include "console.h"

FILE* mre_fopen(char const* _FileName, char const* _Mode) {
	if (_FileName[0] == '.' && _FileName[1] == '\\')
		_FileName += 2;

	VMCHAR wstr[260];
	vm_ascii_to_ucs2(wstr, 260 * 2, _FileName);

	int mode = 0;
	if (_Mode[0] == 'r')
		mode = MODE_READ;
	if (_Mode[0] == 'w')
		mode = MODE_CREATE_ALWAYS_WRITE;

	int h = vm_file_open(wstr, mode, 1);
	if (h < 0)
		return 0;
	return (FILE*)h;
}

size_t mre_fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream) {
	VMUINT nread = 0;
	return vm_file_read((int)_Stream, _Buffer, _ElementSize * _ElementCount, &nread);
}

size_t mre_fwrite(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream) {
	VMUINT written = 0;
	return vm_file_write((int)_Stream, _Buffer, _ElementSize * _ElementCount, &written);
}

long mre_ftell(FILE* _Stream) {
	return vm_file_tell((int)_Stream);
}

int mre_fseek(FILE* _Stream, long _Offset, int _Origin) {
	return vm_file_seek((int)_Stream, _Offset, _Origin + 1);
}

int mre_fclose(FILE* _Stream) {
	vm_file_close((int)_Stream);
	return 0;
}

int mre_fflush(FILE* _Stream) {
	return 0;
}

int mre_mkdir(char const* _Path) {
	VMCHAR wstr[260];
	vm_ascii_to_ucs2(wstr, 260 * 2, _Path);

	return 0;//vm_file_mkdir(wstr);
}

int mre_putchar(int _Character) {
	console_put_char(_Character);
}

int mre_puts(char const* _Buffer) {
	console_put_str(_Buffer);
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