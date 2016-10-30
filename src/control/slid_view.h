#ifndef _SLID_VIEW_H
#define _SLID_VIEW_H

#include "mg_common_header.h"


typedef struct  _SlidView{
	RECT rc;
	HWND hwnd;
	void (*moveTo)(void *object, RECT rc);
	int (*getCurrentScene)(void *object);
	int (*getPrevScene)(void *object);
	int (*getNextScene)(void *object);
	int (*getSceneNum)(void *object);
	
	void *opaque;
} SlidView;


void slidViewMoveTo(void *object, RECT rc);

#endif


