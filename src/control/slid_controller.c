/*
 * =====================================================================================
 *
 *       Filename:  slid_controller.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/11/2016 02:01:39 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  learningx
 *   Organization:  Allwinnertech
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include "slid_controller.h"

static void animationCallback (MGEFF_ANIMATION handle, void* target, int id, void* value)
{
	RECT view_rc, move_rc;
	SlidView* view;

	int* cur_pos = (int*)value;
	SlidController* controller = (SlidController*)target;
	if (NULL == controller)  {
		return;
	}

	view = controller->view;
	view_rc = view->rc;

	SetRect (&move_rc, 0, *cur_pos, RECTW(view_rc), (*cur_pos)+ RECTH(view_rc));
	
	//printf ("scroll animation:(%d, %d, %d, %d)\n", move_rc.left, move_rc.top, move_rc.right, move_rc.bottom);
	
	view->moveTo(view,move_rc);
	//view->invalidateRect(&view->getAnimationArea());
}

SlidController *slidControllerCreate(SlidView *view)
{
	mGEffInit();
	SlidController *controller = (SlidController *)calloc(1,sizeof(SlidController));
	if (!controller) {
		printf("no mem!\n");
		return NULL;
	}
	
	controller->animation = mGEffAnimationCreate (controller, animationCallback, 0, MGEFF_INT);
	controller->view = view;
    controller->buttonPressed = 0;
	
	return controller;
}

int getCurrentScene(void *object)
{
	SlidView *view = (SlidView *)object;
	scene_t *scene = view->opaque;
	
	return scene->cur_idx;
}

int getPrevScene(void *object) {
	SlidView *view = (SlidView *)object;
	scene_t *scene = view->opaque;
	
    if (--scene->cur_idx < 0) {
        scene->cur_idx = 0;
    }
    return scene->cur_idx;
}

int getNextScene(void *object) {
	SlidView *view = (SlidView *)object;
	scene_t *scene = view->opaque;
	
    if (++scene->cur_idx >= scene->num) {
        scene->cur_idx = scene->num-1;
    }
	
    return scene->cur_idx;
}

int getSceneNum(void *object)
{
	SlidView *view = (SlidView *)object;
	scene_t *scene = view->opaque;
	
	return scene->num;
}

void slidViewMoveTo(void *object, RECT rc)
{
	SlidView *view = (SlidView *)object;
	view->rc = rc;
	
	//printf("rc {%d %d %d %d} hwnd:%x\n",rc.left,rc.top,rc.right,rc.bottom,view->hwnd);
	
	InvalidateRect(view->hwnd,NULL,TRUE);
	//UpdateWindow(view->hwnd,TRUE);
}

