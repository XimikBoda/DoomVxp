#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include "vm4res.h"
#include "vmres.h"
#include "vmtimer.h"

#include "doomgeneric.h"

VMINT		layer_hdl[1];	// layer handle array. 
VMUINT8* layer_buf = 0;

VMINT screen_w = 0;
VMINT screen_h = 0;

void handle_sysevt(VMINT message, VMINT param); // system events 
void handle_keyevt(VMINT event, VMINT keycode); // key events 

void DG_Init() {}
void DG_DrawFrame() {
	VMUINT16* layer_buf16 = (VMUINT16*)layer_buf;
	for (int y = 0; y < DOOMGENERIC_RESY; ++y)
		for (int x = 0; x < DOOMGENERIC_RESX; ++x) {
			uint32_t c = DG_ScreenBuffer[y * DOOMGENERIC_RESX + x];
			uint8_t* cc = (uint8_t*)&c;
			layer_buf16[y + (DOOMGENERIC_RESX - x) * 240] = VM_COLOR_888_TO_565(cc[2], cc[1], cc[0]);
		}
	vm_graphic_flush_layer(layer_hdl, 1);
}
void DG_SleepMs(uint32_t ms) {}

uint32_t DG_GetTicksMs() {
	return vm_get_tick_count();
}

int DG_GetKey(int* pressed, unsigned char* key) {
	return 0;
}

void DG_SetWindowTitle(const char* title) {}

void timer(int tid) {
	doomgeneric_Tick();
}


void vm_main(void) {
	layer_hdl[0] = -1;
	screen_w = vm_graphic_get_screen_width();
	screen_h = vm_graphic_get_screen_height();
	
	//vm_reg_sysevt_callback(handle_sysevt);
	vm_reg_keyboard_callback(handle_keyevt);

	layer_hdl[0] = vm_graphic_create_layer(0, 0, screen_w, screen_h, -1);
	layer_buf = vm_graphic_get_layer_buffer(layer_hdl[0]);
	vm_graphic_set_clip(0, 0, screen_w, screen_h);

	char* argv[3] = { 0, "-iwad", "C:\\DOOM.WAD" };
	doomgeneric_Create(3, argv);

	vm_create_timer(1, timer);
}

void handle_sysevt(VMINT message, VMINT param) {
#ifdef		SUPPORT_BG
	switch (message) {
	case VM_MSG_CREATE:
		break;
	case VM_MSG_PAINT:
		layer_hdl[0] = vm_graphic_create_layer(0, 0, screen_w, screen_h, -1);

		layer_buf = vm_graphic_get_layer_buffer(layer_hdl[0]);
		
		vm_graphic_set_clip(0, 0, screen_w, screen_h);
		
		break;
	case VM_MSG_HIDE:	
		if( layer_hdl[0] != -1 )
		{
			vm_graphic_delete_layer(layer_hdl[0]);
			layer_hdl[0] = -1;
		}
		break;
	case VM_MSG_QUIT:
		if( layer_hdl[0] != -1 )
		{
			vm_graphic_delete_layer(layer_hdl[0]);
			layer_hdl[0] = -1;
		}
		break;
	}
#else
	switch (message) {
	case VM_MSG_CREATE:
	case VM_MSG_ACTIVE:
		layer_hdl[0] = vm_graphic_create_layer(0, 0, screen_w, screen_h, -1);

		layer_buf = vm_graphic_get_layer_buffer(layer_hdl[0]);

		vm_graphic_set_clip(0, 0, screen_w, screen_h);
		break;
		
	case VM_MSG_PAINT:
		break;
		
	case VM_MSG_INACTIVE:
		if( layer_hdl[0] != -1 )
			vm_graphic_delete_layer(layer_hdl[0]);
		
		break;	
	case VM_MSG_QUIT:
		if( layer_hdl[0] != -1 )
			vm_graphic_delete_layer(layer_hdl[0]);
		break;	
	}
#endif
}

void handle_keyevt(VMINT event, VMINT keycode) {
}