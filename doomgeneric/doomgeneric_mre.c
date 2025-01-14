#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include "vm4res.h"
#include "vmres.h"
#include "vmtimer.h"

#include "console.h"

#include "doomgeneric.h"
#include "doomkeys.h"

#include <bitstream.h>

VMINT		layer_hdl[1];	// layer handle array. 
VMUINT8* layer_buf = 0;

VMINT screen_w = 0;
VMINT screen_h = 0;

VMBOOL first_frame = 1;

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(unsigned int key) {
	switch (key)
	{
	case VM_KEY_OK:
		key = KEY_ENTER;
		break;
	case VM_KEY_RIGHT_SOFTKEY:
		key = KEY_ESCAPE;
		break;
	case VM_KEY_UP:
	case VM_KEY_NUM1:
		key = KEY_LEFTARROW;
		break;
	case VM_KEY_DOWN:
	case VM_KEY_NUM7:
		key = KEY_RIGHTARROW;
		break;
	case VM_KEY_RIGHT:
	case VM_KEY_NUM5:
		key = KEY_UPARROW;
		break;
	case VM_KEY_LEFT:
	case VM_KEY_NUM4:
		key = KEY_DOWNARROW;
		break;
	case VM_KEY_LEFT_SOFTKEY:
		key = KEY_FIRE;
		break;
	case VM_KEY_NUM2:
		key = VM_KEY_SPACE;
		break;
	default:
		key = 0;
		break;
	}

	return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode) {
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
		//key queue is empty
		return 0;
	}
	else {
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*doomKey = keyData & 0xFF;

		return 1;
	}

	return 0;
}

void handle_keyevt(VMINT event, VMINT keycode) {
	addKeyToQueue(event != VM_KEY_EVENT_UP, keycode);
}

void flush_layer() {
	vm_graphic_flush_layer(layer_hdl, 1);
}

void DG_Init() {}
void DG_DrawFrame() {
	if (first_frame) {
		console_init(DOOMGENERIC_RESY - screen_w, screen_h, layer_buf);
		first_frame = 0;
	}

	VMUINT16* layer_buf16 = (VMUINT16*)layer_buf;
	VMUINT16* game_buf16 = (VMUINT16*)DG_ScreenBuffer;
	for (int y = 0; y < DOOMGENERIC_RESY; ++y)
		for (int x = 0; x < DOOMGENERIC_RESX; ++x) {
			VMUINT16 c = game_buf16[y * DOOMGENERIC_RESX + x];
			layer_buf16[240 - 1 - y + (x) * 240] = c;
		}
	flush_layer();
}
void DG_SleepMs(uint32_t ms) {}

uint32_t DG_GetTicksMs() {
	return vm_get_tick_count();
}

void DG_SetWindowTitle(const char* title) {}

void timer(int tid) {
	doomgeneric_Tick();
}

void handle_sysevt(VMINT message, VMINT param) {
	if (message == VM_MSG_QUIT) {
		bitstream_close();
	}
}


void vm_main(void) {
	layer_hdl[0] = -1;
	screen_w = vm_graphic_get_screen_width();
	screen_h = vm_graphic_get_screen_height();

	vm_reg_keyboard_callback(handle_keyevt);
	vm_reg_sysevt_callback(handle_sysevt);

	layer_hdl[0] = vm_graphic_create_layer(0, 0, screen_w, screen_h, -1);
	layer_buf = vm_graphic_get_layer_buffer(layer_hdl[0]);
	vm_graphic_set_clip(0, 0, screen_w, screen_h);

	console_init(screen_w, screen_h, layer_buf);

	vm_switch_power_saving_mode(turn_off_mode);
	vm_kbd_set_mode(VM_KEYPAD_2KEY_NUMBER);

	char* argv[3] = { 0, "-iwad", "E:\\DOOM.WAD" };
	doomgeneric_Create(3, argv);

	vm_create_timer(1, timer);
}

