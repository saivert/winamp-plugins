/* IconList functions
** New in v0.512: The functions are now explained in detail.
*/

#ifndef _ICONLIST_H
#define _ICONLIST_H 1

#include "NxSThingerAPI.h"

/* Start of C++ check */
#ifdef __cplusplus
extern "C" {
#endif

/* Internal icon list structure.
** Used to maintain the list and not the icons in the list.
** Icons are maintained by a NxSThingerIconStruct structure.
*/
typedef struct _ICONLIST {
  void **listptr;
  int listsize;
} ICONLIST;

/* IconList_Init
** Initializes the list. This function Sets up a memory buffer
** for the list and fills in a ICONLIST structure.
*/
int IconList_Init(void);

/* IconList_Free
** Frees the icon list. Deallocates the list pointer.
*/
void IconList_Free(void);

/* IconList_GetSize
** Returns the size of the list. This is synonymous with the
** number of icons in the list.
*/
int IconList_GetSize(void);

/* IconList_Add
** Adds an icon to the icon list and returns the index of the new icon.
** The index can be used with the IconList_Get and IconList_Del functions.
** Note: Each icon also has an ID which is stored in the icon structure.
*/
int IconList_Add(lpNxSThingerIconStruct i);

/* IconList_Get
** Returns the pointer to the structure for the icon with the given index.
*/
lpNxSThingerIconStruct IconList_Get(int w);

/* IconList_GetFromID
** Returns the pointer to the structure for the icon with the given id.
*/
lpNxSThingerIconStruct IconList_GetFromID(UINT id);

/* IconList_Del
** Deletes the icon and frees the memory occupied by the icon with the given index.
*/
void IconList_Del(int idx);

/* IconList_DelWithID
** Deletes the icon and frees the memory occupied by the icon with the given id.
*/
void IconList_DelWithID(UINT id);


/* End of C++ check */
#ifdef __cplusplus
}
#endif


#endif // _ICONLIST_H
