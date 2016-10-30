/*
** $Id: wheelview.c 13586 2016-10-20 07:01:52Z learningx $
**
** wheelview.c: a wheelview control extent to scrollview control
**
** Copyright (C) 2004 ~ 2008 Feynman Software.
**
** All rights reserved by Allwinnertech Software.
** 
** Current maintainer: Yan Xiaowei
**
** Create date: 2016/10/01
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"

#ifdef _MGCTRL_SCROLLVIEW
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "ctrl/ctrlhelper.h"
#include "ctrl/wheelview.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"

#include "ctrlmisc.h"
#include "scrolled.h"
#include "wheelview_impl.h"
#include "slid_controller.h"
#include "mspeedmeter.h"

/* ------------------------------------ svlist --------------------------- */

static PSVITEMDATA svlistItemAdd (PSVLIST psvlist, PSVITEMDATA preitem, 
                PSVITEMDATA nextitem, PSVITEMINFO pii, int *idx)
{
    PSVITEMDATA ci;
    int ret;

    ci = (PSVITEMDATA) malloc (sizeof(SVITEMDATA));
    if (!ci)
        return NULL;

    if (pii) {
        if (pii->nItemHeight <= 0) 
            ci->nItemHeight = psvlist->nDefItemHeight;
        else
            ci->nItemHeight = pii->nItemHeight;

        ((MgItem *)ci)->addData = pii->addData;
    }
    else {
        ci->nItemHeight = psvlist->nDefItemHeight;
        ((MgItem *)ci)->addData = 0;
    }

    ret = mglist_add_item ( (MgList *)psvlist, (MgItem *)ci, (MgItem *)preitem, 
                      (MgItem *)nextitem, pii->nItem, idx );
    if (ret < 0) {
        free (ci);
        return NULL;
    }

    return ci;
}

static int svlistItemDel (PSVLIST psvlist, PSVITEMDATA pci)
{
    mglist_del_item ((MgList *)psvlist, (MgItem *)pci);
    free (pci);
    return 0;
}

static void svlist_reset_content (HWND hWnd, PSVLIST psvlist)
{
    PSVITEMDATA pci;

    while (!mglist_empty((MgList *)psvlist)) {
        pci = (PSVITEMDATA)mglist_first_item ((MgList *)psvlist);
        svlistItemDel (psvlist, pci);
    }
    ((MgList *)psvlist)->nTotalItemH = 0;
}

//FIXME, to be moved to listmodel.c
static int isInItem (MgList *mglst, int mouseX, int mouseY, 
                     MgItem **pRet, int *item_y)
{
    //PSVLIST psvlist = (PSVLIST)mglst;
    int ret = 0;
    PSVITEMDATA pci = NULL;
    int h = 0;
    list_t *me;

    if (mouseY < 0 || mouseY >= mglst->nTotalItemH)
        return -1;

    mglist_for_each (me, mglst) {
        pci = (PSVITEMDATA)mglist_entry (me);
        h += pci->nItemHeight;
        if (h > mouseY)
            break;
        ret++;
    }

    if (pRet)
        *pRet = (MgItem *)pci;
    if (item_y)
        *item_y = h - pci->nItemHeight;

    return ret;
}

int wheelview_is_in_item (PSVDATA psvdata, int mousey, HSVITEM *phsvi, int *itemy)
{
    return isInItem ((MgList *)&psvdata->svlist, 0, mousey, (MgItem **)phsvi, itemy);
}

/* ---------------------------------------------------------------------------- */

HSVITEM wheelview_add_item (HWND hWnd, PSVDATA psvdata, HSVITEM prehsvi, PSVITEMINFO pii, int *idx)
{
    PSVLIST psvlist = &psvdata->svlist;
    PSVITEMDATA pci;
    int index;

    if ( (pci =svlistItemAdd (psvlist, (PSVITEMDATA)prehsvi, NULL, pii, &index)) ) {
        mglist_adjust_items_height (hWnd, (MgList *)psvlist, psvscr, pci->nItemHeight);
    }

    if (idx)
        *idx = index;

    return (HSVITEM)pci;
}

//FIXME
HSVITEM wheelview_add_item_ex (HWND hWnd, PSVDATA psvdata, HSVITEM prehsvi, 
                HSVITEM nexthsvi, PSVITEMINFO pii, int *idx)
{
    PSVLIST psvlist = &psvdata->svlist;
    PSVITEMDATA pci;
    int index;

    if ( (pci =svlistItemAdd (psvlist, (PSVITEMDATA)prehsvi, 
                      (PSVITEMDATA)nexthsvi, pii, &index)) ) {
        mglist_adjust_items_height (hWnd, (MgList *)psvlist, psvscr, pci->nItemHeight);
    }

    if (idx)
        *idx = index;

    return (HSVITEM)pci;
}

int wheelview_move_item (PSVDATA psvdata, HSVITEM hsvi, HSVITEM prehsvi)
{
    PSVITEMDATA pci, preitem;

    if (!hsvi)
        return -1;
    pci = (PSVITEMDATA)hsvi;
    preitem = (PSVITEMDATA)prehsvi;

    mglist_move_item ((MgList *)&psvdata->svlist, (MgItem *)pci, (MgItem *)preitem);
    wheelview_refresh_content (psvdata);
    return 0;
}

int wheelview_del_item (HWND hWnd, PSVDATA psvdata, int nItem, HSVITEM hsvi)
{
    PSVLIST psvlist = &psvdata->svlist;
    PSVITEMDATA pci;
    int del_h;

    if (hsvi)
        pci = (PSVITEMDATA)hsvi;
    else {
        if ( !(pci=(PSVITEMDATA)mglist_getitem_byindex((MgList *)psvlist, nItem)) )
            return -1;
    }
    del_h = pci->nItemHeight;

    if (svlistItemDel (psvlist, pci) >= 0) {
        mglist_adjust_items_height (hWnd, (MgList *)psvlist, psvscr, -del_h);
    }
    return 0;
}

DWORD wheelview_get_item_adddata (HSVITEM hsvi)
{
    return mglist_get_item_adddata (hsvi);
}   

int wheelview_get_item_index (HWND hWnd, HSVITEM hsvi)
{
    PSVDATA psvdata = (PSVDATA)GetWindowAdditionalData2(hWnd);
    return mglist_get_item_index ((MgList *)&psvdata->svlist, (MgItem *)hsvi);
}

int wheelview_is_item_hilight (HWND hWnd, HSVITEM hsvi)
{
    PSVDATA psvdata = (PSVDATA)GetWindowAdditionalData2(hWnd);
    return (int)mglist_is_item_hilight ((MgList *)&psvdata->svlist, (MgItem *)hsvi);
}

int wheelview_is_item_selected (HSVITEM hsvi)
{
    return mglist_is_item_selected ((MgItem *)hsvi);
}

static int svGetItemRect (PSVDATA psvdata, HSVITEM hsvi, RECT *rcItem, BOOL bConv)
{
    PSVITEMDATA pci = (PSVITEMDATA)hsvi;

    if (!rcItem || !hsvi)
        return -1;

    rcItem->top = wheelview_get_item_ypos (psvdata, hsvi);
    if (bConv)
        scrolled_content_to_window ((PSCRDATA)psvdata, NULL, &rcItem->top);
    rcItem->bottom = rcItem->top + pci->nItemHeight;

    rcItem->left = 0;
    if (bConv)
        scrolled_content_to_window ((PSCRDATA)psvdata, &rcItem->left, NULL);
//FIXME
    rcItem->right = rcItem->left + MAX(psvscr->nContWidth, psvscr->visibleWidth);

    /* only y position is important */
    return 1;
}

int wheelview_get_item_rect (HWND hWnd, HITEM hsvi, RECT *rcItem, BOOL bConv)
{
    PSVDATA psvdata = (PSVDATA)GetWindowAdditionalData2(hWnd);
    return svGetItemRect (psvdata, hsvi, rcItem, bConv);
}

int wheelview_get_item_ypos (PSVDATA psvdata, HSVITEM hsvi)
{
    PSVLIST psvlist = &psvdata->svlist;
    PSVITEMDATA ci = (PSVITEMDATA)hsvi, pci;
    list_t *me;
    int h = 0;

    mglist_for_each (me, psvlist) {
        pci = (PSVITEMDATA)mglist_entry (me);
        if (pci == ci)
            return h;
        h += pci->nItemHeight;
    }
    return -1;
}

int wheelview_set_item_height (HWND hWnd, HSVITEM hsvi, int height)
{
    PSVDATA psvdata = (PSVDATA)GetWindowAdditionalData2(hWnd);
    PSVITEMDATA pci = (PSVITEMDATA)hsvi;
    int diff;

    if (!psvdata || !pci || height < 0 || height == pci->nItemHeight) return -1;

    diff = height - pci->nItemHeight;
    pci->nItemHeight = height;

    mglist_adjust_items_height (hWnd, (MgList *)&psvdata->svlist, psvscr, diff);

    return height - diff;
}

SVITEM_DRAWFUNC
wheelview_set_itemdraw (PSVDATA psvdata, SVITEM_DRAWFUNC draw_func)
{
    SVITEM_DRAWFUNC oldfn;

    oldfn = mglist_set_itemdraw((MgList *)&psvdata->svlist, draw_func);
    wheelview_refresh_content (psvdata);

    return oldfn;
}

void wheelview_reset_content (HWND hWnd, PSVDATA psvdata)
{
    /* delete all svlist content */
    //svlist_reset_content (hsvwnd, &psvdata->svlist);

    if (psvscr->sbPolicy != SB_POLICY_ALWAYS) {
        ShowScrollBar (hWnd, SB_HORZ, FALSE);
        ShowScrollBar (hWnd, SB_VERT, FALSE);
    }

    /* reset content and viewport size */
    scrolled_init_contsize (hWnd, psvscr);
    /* reset svlist window */
    //scrollview_set_svlist (hWnd, psvscr);
    /* reset viewport window */
    scrolled_set_visible (hWnd, psvscr);

    scrolled_set_hscrollinfo (hWnd, psvscr);
    scrolled_set_vscrollinfo (hWnd, psvscr);

    /* delete all svlist content */
    svlist_reset_content (hsvwnd, &psvdata->svlist);
    //FIXME
    //scrollview_refresh_content (psvdata);
    InvalidateRect (hWnd, NULL, TRUE);
}

/* -------------------------------------------------------------------------- */

void wheelview_draw (HWND hWnd, HDC hdc, PSVDATA psvdata)
{
    list_t *me;
    PSVITEMDATA pci;
    RECT rcDraw;
    int h = 0;
    RECT rcVis;
    PSVLIST psvlist = &psvdata->svlist;

    rcDraw.left = 0;
    rcDraw.top = 0;
    rcDraw.right = MAX(psvscr->nContWidth, psvscr->visibleWidth);
    rcDraw.bottom = MAX(psvscr->nContHeight, psvscr->visibleHeight);

    scrolled_content_to_window (psvscr, &rcDraw.left, &rcDraw.top);
    scrolled_content_to_window (psvscr, &rcDraw.right, &rcDraw.bottom);

    scrolled_get_visible_rect (psvscr, &rcVis);
    ClipRectIntersect (hdc, &rcVis);

    mglist_for_each (me, psvlist) {
        pci = (PSVITEMDATA)mglist_entry (me);
        rcDraw.top += h;
        rcDraw.bottom = rcDraw.top + pci->nItemHeight;
        if (rcDraw.bottom < rcVis.top) {
            h = pci->nItemHeight;
            continue;
        }
 //       if (rcDraw.top > rcVis.bottom)
 //           break;
        if (((MgList *)psvlist)->iop.drawItem && pci->nItemHeight > 0) {
            ((MgList *)psvlist)->iop.drawItem (hWnd, (HSVITEM)pci, hdc, &rcDraw);
        }
        h = pci->nItemHeight;
    }
}

/* adjust the position and size of the svlist window */
void wheelview_set_svlist (HWND hWnd, PSCRDATA pscrdata, BOOL visChanged)
{
    PSVDATA psvdata = (PSVDATA) GetWindowAdditionalData2 (hWnd);

    if (visChanged)
        InvalidateRect (hWnd, NULL, TRUE);
    else {
        //should be ScrollWindow ?
        //scrollview_refresh_content (psvdata);
        scrolled_refresh_view (&psvdata->scrdata);
    }
}

static int svlist_init (HWND hWnd, PSVLIST psvlist)
{
    mglist_init((MgList *)psvlist, hWnd);

    psvlist->nDefItemHeight = SV_DEF_ITEMHEIGHT;

    ((MgList *)psvlist)->iop.getRect = wheelview_get_item_rect;
    ((MgList *)psvlist)->iop.isInItem = isInItem;

    return 0;
}

/*
 * initialize scrollview internal structure
 */
static int svInitData (HWND hWnd, PSVDATA psvdata)
{
    RECT rcWnd;

    GetClientRect (hWnd, &rcWnd);
    scrolled_init (hWnd, psvscr, 
                    RECTW(rcWnd) - SV_LEFTMARGIN - SV_RIGHTMARGIN,
                    RECTH(rcWnd) - SV_TOPMARGIN - SV_BOTTOMMARGIN);
    scrolled_init_margins (psvscr, SV_LEFTMARGIN, SV_TOPMARGIN, 
                           SV_RIGHTMARGIN, SV_BOTTOMMARGIN);

    svlist_init (hWnd, &psvdata->svlist);

    psvscr->move_content = wheelview_set_svlist; 
    //psvdata->flags = 0;

    return 0;
}

/* 
 * shoulded be called before scrollview is used
 * hWnd: the scrolled window
 */
int wheelview_init (HWND hWnd, PSVDATA psvdata)
{
    if (!psvdata)
        return -1;

    SetWindowAdditionalData2 (hWnd, 0);
    //ShowScrollBar (hWnd, SB_HORZ, FALSE);
    //ShowScrollBar (hWnd, SB_VERT, FALSE);

    svInitData (hWnd, psvdata);
    SetWindowAdditionalData2 (hWnd, (DWORD) psvdata);

    /* set scrollbar status */
    scrolled_set_hscrollinfo (hWnd, psvscr);
    scrolled_set_vscrollinfo (hWnd, psvscr);

    return 0;
}

/*
 * destroy a scrollview
 */
void wheelview_destroy (PSVDATA psvdata)
{
    svlist_reset_content (hsvwnd, &psvdata->svlist);
}

struct COLOR_LEVEL {
    int isCurColor;
    DWORD color;
};

void draw_highlight_line(HDC hdc, PSVDATA psvdata)
{
    RECT rect;
    SlidController *controller;
    PSCRDATA pscrdata = (PSCRDATA)psvdata;
    controller= pscrdata->controller;
    rect.top = controller->default_item_num * controller->ITEM_HEIGHT;
    rect.bottom = rect.top + controller->ITEM_HEIGHT;
    rect.left = 0;
    rect.right = MAX(psvscr->nContWidth, psvscr->visibleWidth);

    SetMapMode(hdc, MM_TEXT);
    SetPenColor(hdc, RGBA2Pixel(hdc, 0x80, 0x80, 0x0, 0xf0));
    if(controller->COLOR_highline)
        SetPenColor(hdc, controller->COLOR_highline);
    MoveTo(hdc, rect.left, rect.top);
    LineTo(hdc, rect.right, rect.top);
    MoveTo(hdc, rect.left, rect.bottom);
    LineTo(hdc, rect.right, rect.bottom);
}
/* -------------------------------------------------------------------------- */

static void draw_wheelview (HWND hWnd, HDC hdc, PSVDATA psvdata)
{
    list_t *me;
    PSVITEMDATA pci;
    RECT rcDraw;
    int h = 0;
    RECT rcVis;
    PSVLIST psvlist = &psvdata->svlist;
    PSCRDATA pscrdata = (PSCRDATA)psvdata;
    SlidView *view = pscrdata->controller->view;
    SlidController *controller = pscrdata->controller;

    rcDraw.left = 0;
    rcDraw.top = 0;
    rcDraw.right = MAX(psvscr->nContWidth, psvscr->visibleWidth);
    rcDraw.bottom = MAX(psvscr->nContHeight, psvscr->visibleHeight);

    POINT pt;
    pt.x = -view->rc.left;
    pt.y = -view->rc.top + controller->default_item_num * controller->ITEM_HEIGHT;
    //printf("pt.x=%d,pt.y=%d\n", pt.x, pt.y);
    SetMapMode(hdc, MM_ANISOTROPIC);
    SetViewportOrg(hdc, &pt);
    scrolled_content_to_window (psvscr, &rcDraw.left, &rcDraw.top);
    scrolled_content_to_window (psvscr, &rcDraw.right, &rcDraw.bottom);

    scrolled_get_visible_rect (psvscr, &rcVis);
    ClipRectIntersect (hdc, &rcVis);

    mglist_for_each (me, psvlist) {
        pci = (PSVITEMDATA)mglist_entry (me);
        rcDraw.top += h;
        rcDraw.bottom = rcDraw.top + pci->nItemHeight;
        if (rcDraw.bottom < rcVis.top) {
            h = pci->nItemHeight;
            continue;
        }
/* 此处每次paint后都将wheelview所有条目都重绘，可优化 */
        if (((MgList *)psvlist)->iop.drawItem && pci->nItemHeight > 0) {
            ((MgList *)psvlist)->iop.drawItem (hWnd, (HSVITEM)pci, hdc, &rcDraw);
        }
        h = pci->nItemHeight;
    }
}

static int getViewSize(SlidController *controller)
{
	int view_size = 0;
    view_size = RECTH(controller->view->rc);

	return view_size;
}

static int get_speed_move_count(float v, int cur_item, int total_item)
{
    int count;
    int max_count;
    if( v < 0 )
    {
        max_count = total_item -1 - cur_item;
        v *= -1;
    }else{
        max_count = cur_item;
    }

    count = v/200;
    if( count < 0 ){
        count = 0;
    }else if(count > 7){
        count = 7;
    }

    if( count > max_count )
    {
        count = max_count;
    }
    return count;
}

static int get_still_move_count(int cur_item, int total_item, SlidController *controller)
{
    int count;
    int max_count;
    int end_pos_y = controller->end_pos_y;
    int start_pos_y = controller->start_pos_y;
    int ITEM_HEIGHT = controller->ITEM_HEIGHT;
    if( end_pos_y < start_pos_y )
    {
        max_count = total_item -1 - cur_item;
    }else{
        max_count = cur_item;
    }

    if(end_pos_y == 0)  end_pos_y = start_pos_y;
    count = abs(end_pos_y - start_pos_y)/ITEM_HEIGHT + ((abs(end_pos_y - start_pos_y)%ITEM_HEIGHT)/(ITEM_HEIGHT/2));
    if( count < 0 )
        count = 0;

    if( count > max_count )
    {
        count = max_count;
    }
    return count;
}

static int get_to_item(int cur_item, int count, int direction)
{
    int to_item = cur_item + count * direction;
    return to_item;
}


static POINT GetWindowLeftPoint(HWND hwnd)
{
    const WINDOWINFO* win_info = GetWindowInfo(hwnd);
    POINT pt;
    pt.x = win_info->left;
    pt.y = win_info->top;
    return pt;
}
/* --------------------------------------------------------------------------------- */
int WheelViewCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    PSVDATA psvdata = NULL;
    PSVLIST psvlist = NULL;
    
    POINT pt = GetWindowLeftPoint(hWnd);
//printf("-----------wheelviewproc----------message:0x%x\n", message);
    if (message != MSG_CREATE) {
        psvdata = (PSVDATA) GetWindowAdditionalData2 (hWnd);
        psvlist = &psvdata->svlist;
    }

    SpeedMeterProc(hWnd, message, wParam, lParam);
    switch (message) {

    case MSG_CREATE:
    {
        psvdata = (PSVDATA) malloc(sizeof (SVDATA));
        if (!psvdata)
            return -1;
        wheelview_init (hWnd, psvdata);
        if (GetWindowStyle(hWnd) & SVS_AUTOSORT) {
            wheelview_set_autosort (psvdata);
        }
        break;
    }

    case MSG_DESTROY:
        wheelview_destroy (psvdata);
        free (psvdata);
        break;
    
    case MSG_GETDLGCODE:
        return DLGC_WANTARROWS;

    case MSG_KEYDOWN:
    {
        HSVITEM hsvi = 0, curHilighted;

        curHilighted = (HSVITEM) mglist_get_hilighted_item((MgList *)&psvdata->svlist);

        if (wParam == SCANCODE_CURSORBLOCKDOWN) {
            hsvi = wheelview_get_next_item(psvdata, curHilighted);
            if (GetWindowStyle(hWnd) & SVS_LOOP && !hsvi) {
                hsvi = wheelview_get_next_item(psvdata, 0);
            }
        }
        else if (wParam == SCANCODE_CURSORBLOCKUP) {
            hsvi = wheelview_get_prev_item(psvdata, curHilighted);
            if (GetWindowStyle(hWnd) & SVS_LOOP && !hsvi) {
                hsvi = wheelview_get_prev_item(psvdata, 0);
            }
        }
        else if (wParam == SCANCODE_HOME) {
            hsvi = wheelview_get_next_item(psvdata, 0);
        }
        else if (wParam == SCANCODE_END) {
            hsvi = wheelview_get_prev_item(psvdata, 0);
        }

        /* skip the invisible items */
        while ( hsvi && ((PSVITEMDATA)hsvi)->nItemHeight <= 0 ) {
            hsvi = (wParam == SCANCODE_CURSORBLOCKDOWN || wParam == SCANCODE_HOME) ? 
                   wheelview_get_next_item(psvdata, hsvi) : wheelview_get_prev_item(psvdata, hsvi);
        }

        if (hsvi) {
            if (hsvi != curHilighted) {
                NotifyParentEx (hWnd, GetDlgCtrlID(hWnd), SVN_SELCHANGING, (DWORD)curHilighted);
                wheelview_hilight_item (psvdata, hsvi);
                NotifyParentEx (hWnd, GetDlgCtrlID(hWnd), SVN_SELCHANGED, (DWORD)hsvi);
            }
            wheelview_make_item_visible (psvdata, hsvi);
        }
        break;
    }

        case MSG_LBUTTONDOWN:
        case MSG_LBUTTONUP:
        {
            int mouseY = HISWORD (lParam) ;    
            int mouseX = LOSWORD (lParam) ;
            int nItem;
            RECT rcVis;
            MgItem *hitem;
            PSCRDATA pscrdata = (PSCRDATA)psvdata;
            SlidView *view = pscrdata->controller->view;
            SlidController *controller = pscrdata->controller;
       
            controller->old_bt_state = controller->buttonPressed;
            controller->buttonPressed = (MSG_LBUTTONDOWN == message);
            controller->mousePosition.x = mouseX;
            controller->mousePosition.y = mouseY;
    
            if( MSG_LBUTTONDOWN == message ) {
                //printf("-----MSG_LBUTTONDOWN-----------mouseY:%d-----\n", mouseY);
                RECT cur_rc = view->rc;
                controller->start_pos_y = mouseY;
                //printf("start=%d\n", controller->start_pos_y);
                
                if(NULL == controller->speedmeter)
                    controller->speedmeter = mSpeedMeter_create(1000, 10);
                mSpeedMeter_reset(controller->speedmeter);
                mSpeedMeter_append(controller->speedmeter, mouseX, mouseY, GetTickCount()*10);
                int cur_scene = view->getCurrentScene(view);
    		    int view_size = getViewSize(controller);
                controller->startvalue = cur_scene * view_size; 
                return 0;
            }   
    
            if( MSG_LBUTTONUP == message ) {
                if(!controller->buttonPressed==1 && controller->old_bt_state == controller->buttonPressed)  return 0;

                mouseX -= pt.x;//坐标转换：lang  to scrollview
                mouseY -= pt.y;
                //printf("-----MSG_LBUTTONUP-----------mouseY:%d-----\n", mouseY);
                mSpeedMeter_append(controller->speedmeter, mouseX, mouseY, GetTickCount()*10);
                float v_x,v_y;
                QueryMouseMoveVelocity(&v_x, &v_y);
                //printf("v_x=%f,v_y=%f\n", v_x, v_y);
                mSpeedMeter_stop(controller->speedmeter);
    
    	        RECT cur_rc = view->rc;
    				
                //move items by still move
                int cur_item = SendMessage(hWnd, SVM_GETCURSEL, 0, 0);
                int total_item = SendMessage(hWnd, SVM_GETITEMCOUNT, 0, 0); 
    		    int view_size = getViewSize(controller);
    		    int boundary_size = view_size>>1; 
    		    int cur_scene = view->getCurrentScene(view);
    		    int page_top = cur_scene * view_size;
    		    int page_bottom = page_top + view_size;

                int count = 0;
                int move_count = get_still_move_count(cur_scene, total_item, controller);
                //printf("----still move-------cur_item=%d,cur_scene=%d,total_item=%d,move_count=%d\n", cur_item,cur_scene,total_item,move_count);
                while(count++ < move_count){
    	            if (cur_rc.top <= page_top-boundary_size) {
    			        cur_scene = view->getPrevScene(view);
    		        }
    		        else if (cur_rc.bottom >= page_bottom+boundary_size) {
    			        cur_scene = view->getNextScene(view);
    		        }else{
                        move_count = 0;
                    }
                } 
                SendMessage(hWnd, SVM_SETCURSEL, cur_scene, 0);

                //move items by sliding speed 
                cur_rc = view->rc;
                page_top = cur_scene * view_size;
                page_bottom = page_top + view_size;
                cur_scene = view->getCurrentScene(view);
                cur_item = SendMessage(hWnd, SVM_GETCURSEL, 0, 0);
                count = 0; move_count = 0;
                move_count = get_speed_move_count(v_y, cur_scene, total_item);
                //printf("------speed move----------cur_item=%d,cur_scene=%d,total_item=%d,move_count=%d\n", cur_item,cur_scene,total_item,move_count);
                while(count++ < move_count){
                    //printf("cur_rc.top = %d,page_top=%d\n", cur_rc.top, page_top);
    	            if ( v_y > 0 ) {
    			        cur_scene = view->getPrevScene(view);
    			        //printf ("move prevPage\n");
    		        }
    		        else if ( v_y < 0 ) {
    			        cur_scene = view->getNextScene(view);
    			        //printf ("move nextPage\n");
    		        }else{
                        move_count = 0;
                    }
                }   
                int to_item,direction;
                SendMessage(hWnd, SVM_SETCURSEL, cur_scene, 0);
                controller->endvalue = cur_scene * view_size;
    		    int duration = 150;
    				//printf("startvalue=%d, endvalue=%d\n",controller->startvalue,controller->endvalue);
    		    if (controller->animation) {
    	            mGEffAnimationSetStartValue (controller->animation, &controller->startvalue);
    			    mGEffAnimationSetEndValue (controller->animation, &controller->endvalue);
    			    mGEffAnimationSetDuration (controller->animation, duration);    
    			    mGEffAnimationAsyncRun (controller->animation);
                    controller->g_distance += controller->startvalue - controller->endvalue;
    		    }
            }
            return 0;
        }

        case MSG_MOUSEMOVE:
        {
            PSCRDATA pscrdata = (PSCRDATA)psvdata;
            SlidView *view = pscrdata->controller->view;
            SlidController *controller = pscrdata->controller;

            int mouse_pos = 0;
            //from parent window
            int x_pos = LOSWORD (lParam) - pt.x;
            int y_pos = HISWORD (lParam) - pt.y;
    
            //printf("pt.x=%d,pt.y=%d\n", pt.x,pt.y);
            //printf("-------buttonPressed=%d\n", controller->buttonPressed);
            if(controller && controller->buttonPressed==1)
            {
                int distance = 0;
                RECT cur_rc;
    
                controller->end_pos_y = y_pos;
                mSpeedMeter_append(controller->speedmeter, x_pos, y_pos, GetTickCount()*10);
                cur_rc = view->rc;
                distance = y_pos - controller->mousePosition.y;
                controller->g_distance += distance;
                //printf("cur_rc.top=%d\n", cur_rc.top);
                SetRect (&controller->move_rc, cur_rc.left, cur_rc.top-distance, cur_rc.right, cur_rc.bottom-distance);
                view->moveTo(view, controller->move_rc);
                controller->startvalue = controller->move_rc.top;
                controller->mousePosition.y = y_pos;
            }

            return 0;
        }
   
        case MSG_PAINT:
        {
            HDC hdc = BeginPaint(hWnd);
            draw_wheelview(hWnd, hdc, psvdata);
            draw_highlight_line(hdc, psvdata);
            EndPaint(hWnd, hdc);
            return 0;
        }

        case SVM_INIT:
        {
            //printf("------------------SVM_INIT-------------\n");
            //wParam:item_height   lParam:item_nums
            PSCRDATA pscrdata = (PSCRDATA)psvdata;
            SlidView *view = &pscrdata->view;
            int item_height = wParam;
            int default_item_num = (pscrdata->visibleHeight+pscrdata->topMargin+pscrdata->bottomMargin)/2/wParam;
            int default_item = default_item_num;
            view->rc.left =0;
    	    view->rc.top = default_item * item_height;
    	    view->rc.right = pscrdata->visibleWidth;
            //get_item_height_and_nums(pscrdata, &item_height, &item_nums);
    	    view->rc.bottom = view->rc.top + item_height;//item height
    	    scene_t *scene = calloc(1,sizeof(scene_t));
    	    if (!scene) {
    		    printf("no mem for scene_t!\n");
    		    return -1;
    	    }
    	
    	    scene->cur_idx = default_item;
    	    scene->num = (int)lParam;//items number
    	    view->opaque = scene;
    
            pscrdata->controller = slidControllerCreate(view);
            pscrdata->controller->ITEM_HEIGHT = item_height;
            pscrdata->controller->default_item_num = default_item_num;
            view->hwnd = hWnd;
    	
    	    view->moveTo = slidViewMoveTo;
    	    view->getCurrentScene = getCurrentScene;
    	    view->getPrevScene = getPrevScene;
    	    view->getNextScene = getNextScene;
    	    view->getSceneNum = getSceneNum;
            
            return 0;
        }
    
    case SVM_GETMOVEDIS:
    {
        PSCRDATA pscrdata = (PSCRDATA)psvdata;
        SlidController *controller = pscrdata->controller;
        return controller->g_distance;
    }

    case SVM_SETHIGHLINECOLOR:
    {
        DWORD color = (DWORD)wParam;
        PSCRDATA pscrdata = (PSCRDATA)psvdata;
        SlidController *controller = pscrdata->controller;
        controller->COLOR_highline = color;
        return 0;
    }

    case SVM_ADDITEM:
    {
        int idx;
        HSVITEM hsvi;

        hsvi = wheelview_add_item (hWnd, psvdata, 0, (PSVITEMINFO)lParam, &idx);
        if (wParam)
            *(HSVITEM *)wParam = hsvi;
        return idx;
    }

    case SVM_DELITEM:
        wheelview_del_item (hWnd, psvdata, wParam, (HSVITEM)lParam);
        return 0;

    case SVM_RESETCONTENT:
        wheelview_reset_content (hWnd, psvdata);
        return 0;

    case SVM_SETITEMHEIGHT:
    {
        int nItem = (int)wParam;
        int new_h = (int)lParam;
        HSVITEM hsvi;

        hsvi = wheelview_get_item_by_index (psvdata, nItem);
        wheelview_set_item_height (hWnd, hsvi, new_h);
        return 0;
    }


    }/* end switch */

    //return DefaultScrolledProc (hWnd, message, wParam, lParam);
    return DefaultItemViewProc (hWnd, message, wParam, lParam, 
                                psvscr, (MgList *)&psvdata->svlist);
}

BOOL RegisterWheelViewControl (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_WHEELVIEW;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = GetWindowElementPixel (HWND_NULL, WE_BGC_WINDOW);
    WndClass.WinProc     = WheelViewCtrlProc;

    return AddNewControlClass (&WndClass) == ERR_OK;
}

#endif /* _MGCTRL_SCROLLVIEW */

