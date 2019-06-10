// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "ashhelloworld.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// ISayHello properties

/////////////////////////////////////////////////////////////////////////////
// ISayHello operations

void ISayHello::Play()
{
	InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Stop()
{
	InvokeHelper(0x2, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Pause()
{
	InvokeHelper(0x3, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Next()
{
	InvokeHelper(0x4, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Prev()
{
	InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::AddToLog(unsigned short* szText)
{
	static BYTE parms[] =
		VTS_PI2;
	InvokeHelper(0x6, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 szText);
}


/////////////////////////////////////////////////////////////////////////////
// ISayHelloEvents properties

/////////////////////////////////////////////////////////////////////////////
// ISayHelloEvents operations

void ISayHelloEvents::OnSongChange()
{
	InvokeHelper(0xa, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// ISayHello properties

/////////////////////////////////////////////////////////////////////////////
// ISayHello operations

void ISayHello::Play()
{
	InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Stop()
{
	InvokeHelper(0x2, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Pause()
{
	InvokeHelper(0x3, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Next()
{
	InvokeHelper(0x4, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::Prev()
{
	InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ISayHello::AddToLog(unsigned short* szText)
{
	static BYTE parms[] =
		VTS_PI2;
	InvokeHelper(0x6, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 szText);
}


/////////////////////////////////////////////////////////////////////////////
// ISayHelloEvents properties

/////////////////////////////////////////////////////////////////////////////
// ISayHelloEvents operations

void ISayHelloEvents::OnSongChange()
{
	InvokeHelper(0xa, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}
