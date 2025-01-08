#include <stdio.h>
#include <vmio.h>
#include <vmchset.h>
#include <vmstdlib.h>


FILE* mre_fopen(char const* _FileName, char const* _Mode) {
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

int mre_mkdir(char const* _Path) {
	VMCHAR wstr[260];
	vm_ascii_to_ucs2(wstr, 260 * 2, _Path);

	return vm_file_mkdir(wstr);
}

int mre_putchar(int _Character) {}

int mre_puts(char const* _Buffer) {}

int mre_printf(char const* const _Format, ...) {}

int mre_fprintf() {}

int mre_vfprintf() {}