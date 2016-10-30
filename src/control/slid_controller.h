#ifndef _SLID_CONTROLER_H
#define _SLID_CONTROLER_H

#include "slid_view.h"
#include "mspeedmeter.h"

typedef struct _scene {
	int cur_idx;
	int num;
} scene_t;

enum emSlidDirection{
	SLID_HORIZONTAL,
	SLID_VERTICAL,
};

typedef struct _SlidController {
	SlidView *view;
	int buttonPressed;
    int old_bt_state;
	POINT mousePosition;
	MGEFF_ANIMATION animation;
	enum emSlidDirection slidDirection; /*0: v 1:h*/
    int start_pos_y;
    int end_pos_y;
    SPEEDMETER speedmeter;
    RECT move_rc;
    int startvalue;
    int endvalue;
    int g_distance;
    int default_item_num;
    int ITEM_HEIGHT;
    DWORD COLOR_highline;
} SlidController;


SlidController *slidControllerCreate(SlidView *view);
int slidControllerWndProc(SlidController *controller , int message, WPARAM wParam, LPARAM lParam);
int slidControllerDestroy(SlidController *controller);

int getCurrentScene(void *object);
int getPrevScene(void *object);
int getNextScene(void *object);
int getSceneNum(void *object);
void slidViewMoveTo(void *object, RECT rc);

#endif

