#ifndef _MG_COMMON_HEADER_H
#define _MG_COMMON_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>

#include <common.h>
#include <minigui.h>
#include <gdi.h>
#include <window.h>
#include <control.h>

#include <mgeff.h>


#ifdef _DEBUG
#define CN_PRINT(format, ...) fprintf (stderr,"[%s:%d]"format, __FUNCTION__,__LINE__,## __VA_ARGS__)
#else
#define CN_PRINT(format, ...) 
#endif

#define CN_ERR (-1)
#define CN_OK (0)


#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offset_of(type,member) );})


#define ARRY_SIZE(arry) (sizeof(arry)/sizeof(arry[0]))


#ifdef __cplusplus
}
#endif

#endif


