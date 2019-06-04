/* Icon list
** This is a quick C conversion of a C++ class,
** so that's why the code looks a bit ugly.
**
** New in v0.512:
**  - Decided to make "pList" staticly allocated and
**    renamed the variable to "g_list".
**  - Moved code for finding icon with a specific id into this module.
**    New functions "IconList_GetFromID" and "IconList_DelWithID" added.
*/

#include <windows.h>
#include "iconlist.h"

//static ICONLIST *pList=0;
static ICONLIST g_list;

int IconList_Init(void) {
	//pList = (ICONLIST*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ICONLIST));
	g_list.listptr = (void**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0);
	g_list.listsize = 0;
	return 1; /* success */
}

void IconList_Free(void) {
	if (g_list.listptr) {
		/* Delete and free all icons */
		while (g_list.listsize) {
			IconList_Del(0);
		}

		/* Free list */
		HeapFree(GetProcessHeap(), 0, g_list.listptr);
		/* Free list structure itself */
		//HeapFree(GetProcessHeap(), 0, pList);
	}
}

int IconList_GetSize(void) {
	if (g_list.listptr)
		return g_list.listsize;
	else
		return 0;
}

int IconList_Add(lpNxSThingerIconStruct i)
{
	static int uIconId = 1;
	lpNxSThingerIconStruct copy;
	//if (!pList) return 0;

	if (!g_list.listptr || !(g_list.listsize&31))
	{
		//m_list=(void**)::realloc(m_list, sizeof(void*) * (m_size+32));
		g_list.listptr=(void**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, g_list.listptr, sizeof(void*) * (g_list.listsize+32));
	}
	copy = (lpNxSThingerIconStruct)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NxSThingerIconStruct));
	CopyMemory(copy, i, sizeof(NxSThingerIconStruct));
	g_list.listptr[g_list.listsize++]=copy;
	return (copy->uIconId = uIconId++); // assign iconid
}

lpNxSThingerIconStruct IconList_Get(int w)
{
	if (!g_list.listptr) return 0;

	if (w >= 0 && w < g_list.listsize)
		return (lpNxSThingerIconStruct)g_list.listptr[w];
	return NULL;
}

lpNxSThingerIconStruct IconList_GetFromID(UINT id)
{
	int i;

	for (i=0; i<g_list.listsize; i++) {
		if (((lpNxSThingerIconStruct)g_list.listptr[i])->uIconId == id)
			return (lpNxSThingerIconStruct)g_list.listptr[i];
	}
	return NULL;
}


void IconList_Del(int idx)
{
	//if (!pList) return;

	if (g_list.listptr && idx >= 0 && idx < g_list.listsize)
	{
		//Free NxSThingerIconStruct
		HeapFree(GetProcessHeap(), 0, g_list.listptr[idx]);


		g_list.listsize--;
		if (idx != g_list.listsize)
			//::memcpy(m_list+idx, m_list+idx+1, sizeof(void *) * (m_size-idx));
			CopyMemory(g_list.listptr+idx, g_list.listptr+idx+1, sizeof(void *) * (g_list.listsize-idx));

		if (!(g_list.listsize&31) && g_list.listsize) // resize down
		{
			//m_list=(void**)::realloc(m_list, sizeof(void*) * m_size);
			g_list.listptr=(void**)HeapReAlloc(GetProcessHeap(), 0, g_list.listptr, sizeof(void*) * g_list.listsize);
		}
	}
}

void IconList_DelWithID(UINT id)
{
	int i;
	for (i=0; i<g_list.listsize; i++) {
		if (((lpNxSThingerIconStruct)g_list.listptr[i])->uIconId == id) {
			IconList_Del(i);
			break;
		}
	}
}
