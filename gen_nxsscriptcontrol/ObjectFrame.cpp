/******************************************************************************
*
*   File:   ObjectFrame.cpp
*
*   Date:   March 3, 1998
*
*   Description:   This file contains the definition of a generic class that
*               implements the IDispatch interface, which allows the 
*               methods of this class to be called by any object that 
*               understands the IDispatch interface.
*
*   Modifications:
******************************************************************************/
#include "stdafx.h"

#include "ObjectFrame.h"
#include "MyDispids.h"
#include <stdio.h>
#include "gen.h"
#include "wa_ipc.h"
#include "wa_msgids.h"

extern char g_szTypeLibFile[MAX_PATH];
extern winampGeneralPurposePlugin plugin;
extern HWND g_hwndScriptDlg;

//Constructor
CObjectFrame::CObjectFrame()
{
	OutputDebugString("CObjectFrame\n");
	m_refCount = 0;
	m_typeInfo = NULL;
	m_theConnection = NULL;
	
	HRESULT hr = LoadTypeInfo(&m_typeInfo, CLSID_ashObject, 0x0);
	if (FAILED(hr))
		OutputDebugString("Error: Can't load type info in CObjectFrame.");
}

//Destructor
CObjectFrame::~CObjectFrame()
{
	if (m_typeInfo != NULL){
		m_typeInfo->Release();
		m_typeInfo = NULL;
	}
	OutputDebugString("~CObjectFrame\n");
}

/******************************************************************************
*   LoadTypeInfo -- Gets the type information of an object's interface from the 
*   type library.  Returns S_OK if successful.
******************************************************************************/
STDMETHODIMP CObjectFrame::LoadTypeInfo(ITypeInfo** pptinfo, REFCLSID clsid,
										LCID lcid)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::LoadTypeInfo\n");
	
	HRESULT hr;
    LPTYPELIB ptlib = NULL;
    LPTYPEINFO ptinfo = NULL;
	*pptinfo = NULL;
	
	
    // First try to load the type info from a registered type library
	hr = LoadRegTypeLib(LIBID_ashHelloWorld, 2, 0, lcid, &ptlib);
    if (FAILED(hr)){
		//if the libary is not registered, try loading from a file
		
		WCHAR wsz[MAX_PATH];
		
		//Copy the file to the script buffer, which is a Unicode string
		if (!MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, g_szTypeLibFile, -1, wsz, MAX_PATH)){
			OutputDebugString("Error translating string. (CObjectFrame::LoadTypeInfo)\n");
			return false;
		}
		
		
		hr = LoadTypeLib(wsz, &ptlib);
		
		if (FAILED(hr)){
			//can't get the type information
			return hr;
		}
	}
	
    // Get type information for interface of the object.
	hr = ptlib->GetTypeInfoOfGuid(clsid, &ptinfo);
    if (FAILED(hr))
    {
		ptlib->Release();
        return hr;
    }
    ptlib->Release();
	*pptinfo = ptinfo;
	return S_OK;
}

/******************************************************************************
*   IUnknown Interfaces -- All COM objects must implement, either directly or 
*   indirectly, the IUnknown interface.
******************************************************************************/

/******************************************************************************
*   QueryInterface -- Determines if this component supports the requested 
*   interface, places a pointer to that interface in ppvObj if it's available,
*   and returns S_OK.  If not, sets ppvObj to NULL and returns E_NOINTERFACE.
******************************************************************************/
STDMETHODIMP CObjectFrame::QueryInterface(REFIID riid, void ** ppvObj)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::QueryInterface->");
	
	if (riid == IID_IUnknown){
		OutputDebugString("IUnknown\n");
		*ppvObj = static_cast<IDispatch*>(this);
	}
	
	else if (riid == IID_IDispatch){
		OutputDebugString("IDispatch\n");
		*ppvObj = static_cast<IDispatch*>(this);
	}
	
	else if (riid == DIID_ISayHello){
		OutputDebugString("ISayHello\n");
		*ppvObj = static_cast<ISayHello*>(this);
	}
	
	else if (riid == IID_IProvideMultipleClassInfo){
		OutputDebugString("IProvideMultipleClassInfo\n");
		*ppvObj = static_cast<IProvideMultipleClassInfo*>(this);
	}
	
	else if (riid == IID_IConnectionPointContainer){
		OutputDebugString("IConnectionPointContainer\n");
		*ppvObj = static_cast<IConnectionPointContainer*>(this);
	}
	
	else if (riid == IID_IConnectionPoint){
		OutputDebugString("IConnectionPoint\n");
		*ppvObj = static_cast<IConnectionPoint*>(this);
	}
	
	else{
		OutputDebugString("Unsupported Interface\n");
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}
	
	static_cast<IUnknown*>(*ppvObj)->AddRef();
	return S_OK;
}

/******************************************************************************
*   AddRef() -- In order to allow an object to delete itself when it is no 
*   longer needed, it is necessary to maintain a count of all references to 
*   this object.  When a new reference is created, this function increments
*   the count.
******************************************************************************/
STDMETHODIMP_(ULONG) CObjectFrame::AddRef()
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::AddRef\n");
	
	return ++m_refCount;
}

/******************************************************************************
*   Release() -- When a reference to this object is removed, this function 
*   decrements the reference count.  If the reference count is 0, then this 
*   function deletes this object and returns 0;
******************************************************************************/
STDMETHODIMP_(ULONG) CObjectFrame::Release()
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Release\n");
	char txt[10];
	sprintf(txt, "%d", m_refCount);
	strcat(txt, "\n");
	OutputDebugString(txt);
	
	if (--m_refCount == 0)
	{
		delete this;
		return 0;
	}
	return m_refCount;
}

/******************************************************************************
*   IDispatch Interface -- This interface allows this class to be used as an
*   automation server, allowing its functions to be called by other COM
*   objects
******************************************************************************/

/******************************************************************************
*   GetTypeInfoCount -- This function determines if the class supports type 
*   information interfaces or not.  It places 1 in iTInfo if the class supports
*   type information and 0 if it doesn't.
******************************************************************************/
STDMETHODIMP CObjectFrame::GetTypeInfoCount(UINT *iTInfo)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetTypeInfoCount\n");
	
	*iTInfo = 1;
	return S_OK;
}

/******************************************************************************
*   GetTypeInfo -- Returns the type information for the class.  For classes 
*   that don't support type information, this function returns E_NOTIMPL;
******************************************************************************/
STDMETHODIMP CObjectFrame::GetTypeInfo(UINT iTInfo, LCID lcid, 
									   ITypeInfo **ppTInfo)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetTypeInfo\n");
	
	m_typeInfo->AddRef();
	*ppTInfo = m_typeInfo;
	
	return S_OK;
}

/******************************************************************************
*   GetIDsOfNames -- Takes an array of strings and returns an array of DISPID's
*   which corespond to the methods or properties indicated.  If the name is not 
*   recognized, returns DISP_E_UNKNOWNNAME.
******************************************************************************/
STDMETHODIMP CObjectFrame::GetIDsOfNames(REFIID riid,  
										 OLECHAR **rgszNames, 
										 UINT cNames,  LCID lcid,
										 DISPID *rgDispId)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetIDsOfNames\n");
	
	HRESULT hr;
	
	//Validate arguments
	if (riid != IID_NULL)
		return E_INVALIDARG;
	
	//this API call gets the DISPID's from the type information
	hr = m_typeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
	
	//DispGetIDsOfNames may have failed, so pass back its return value.
	return hr;
}

/******************************************************************************
*   Invoke -- Takes a dispid and uses it to call another of this class's 
*   methods.  Returns S_OK if the call was successful.
******************************************************************************/
STDMETHODIMP CObjectFrame::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
								  WORD wFlags, DISPPARAMS* pDispParams,
								  VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
								  UINT* puArgErr)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Invoke\n");

	//Validate arguments
	if (riid != IID_NULL)
		return E_INVALIDARG;
	
	// This is supposed to work ,but it doesn't.  It may not be implemented.
	/*	
	HRESULT hr = m_typeInfo->Invoke((ISayHello*)this, dispIdMember, wFlags, 
		pDispParams, pVarResult, pExcepInfo, puArgErr);
	*/
	

	HRESULT hr = S_OK;


	if (wFlags & DISPATCH_PROPERTYGET) {
		switch(dispIdMember) {
		case 0x1:
			pVarResult->vt = VT_I2;
			pVarResult->iVal = GetVersion();
			break;
		case 0x2:
			pVarResult->vt = VT_I2;
			pVarResult->iVal = GetVolume();
			break;
		case 0x3:
			pVarResult->vt = VT_I2;
			pVarResult->iVal = GetPanning();
			break;
		default:
			hr = E_FAIL;
			break;
		}
	} else if (wFlags & DISPATCH_PROPERTYPUT) {
		switch(dispIdMember) {
		case 0x1:
			SetVersion();
			break;
		case 0x2:
			SetVolume(pDispParams->rgvarg[0].iVal);
			break;
		case 0x3:
			SetPanning(pDispParams->rgvarg[0].iVal);
			break;
		default:
			hr = E_FAIL;
			break;
		}
	} else if (wFlags & DISPATCH_METHOD) {
		switch(dispIdMember) {
		case 0x4:
			AddToLog(pDispParams->rgvarg[0].bstrVal);
			break;
		case 0x5:
			Play();
			break;
		case 0x6:
			Stop();
			break;
		case 0x7:
			Pause();
			break;
		case 0x8:
			Next();
			break;
		case 0x9:
			Prev();
			break;
		case 0xA:
			pVarResult->vt = VT_I2;
			pVarResult->iVal = GetPlaylistLength();
			break;
		default:
			hr = E_FAIL;
			break;
		}
	} else return E_INVALIDARG;

	return hr;
}


/******************************************************************************
*   IProvideClassInfo method -- GetClassInfo -- this method returns the 
*   ITypeInfo* that corresponds to the objects coclass type information.
******************************************************************************/
STDMETHODIMP CObjectFrame::GetClassInfo( ITypeInfo** ppTI )
{   
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetClassInfo\n");
	
	return LoadTypeInfo( ppTI, CLSID_ashObject, 0);
}

/******************************************************************************
*   IProvideClassInfo2 method -- GetGUID -- returns the IID of the object's
*   default outgoing event set.
******************************************************************************/
STDMETHODIMP CObjectFrame::GetGUID( DWORD dwGuidKind, GUID* pGUID)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetGUID\n");
	
	if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
		return E_INVALIDARG;
	else{
		*pGUID = DIID_ISayHello;
		return S_OK;
	}
}

/******************************************************************************
*   IProvideMultipleClassInfo methods -- provides access to type information
*   for the default interfaces of this class
******************************************************************************/

/******************************************************************************
*   GetMultiTypeInfoCount -- returns the number of type information interfaces
*   that make up the composite interface
******************************************************************************/
STDMETHODIMP CObjectFrame::GetMultiTypeInfoCount(ULONG *pcti)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetMultiTypeInfoCount\n");
	
	*pcti = 1;
	return S_OK;
}

/******************************************************************************
*   GetInfoOfIndex -- returns information associated with a particular 
*   interface in this composite class.
******************************************************************************/
STDMETHODIMP CObjectFrame::GetInfoOfIndex(ULONG iti, DWORD dwMCIFlags, 
										  ITypeInfo **pptiCoClass, 
										  DWORD *pdwTIFlags, 
										  ULONG *pcdispidReserved, 
										  IID *piidPrimary, IID *piidSource)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetInfoOfIndex\n");
	
	if (iti != 0)
		return E_INVALIDARG;
	
	if (dwMCIFlags & MULTICLASSINFO_GETTYPEINFO){
		LoadTypeInfo(pptiCoClass, CLSID_ashObject, 0);
	}
	
	if (dwMCIFlags & MULTICLASSINFO_GETNUMRESERVEDDISPIDS){
		*pdwTIFlags = 0;
		*pcdispidReserved = 0;
	}
	
	if (dwMCIFlags & MULTICLASSINFO_GETIIDPRIMARY){
		*piidPrimary =  DIID_ISayHello;
	}
	
	if (dwMCIFlags & MULTICLASSINFO_GETIIDSOURCE){
		*piidSource = DIID_ISayHello;
	}
	
	return S_OK;
}

/******************************************************************************
*   IConnectionPointContainer methods -- These methods support connection 
*   points for use with outgoing interfaces.
******************************************************************************/

/******************************************************************************
*   EnumConnectionPoints -- provides an enumeration of the connection points
*    that this object supports.
******************************************************************************/
STDMETHODIMP CObjectFrame::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::EnumConnectionPoints\n");
	
	return E_NOTIMPL;
}

/******************************************************************************
*   FindConnectionPoint -- This function returns the IConnectionPoint* 
*   specified by riid.  Returns S_OK if successful, CONNECT_E_NOCONNECTION if
*   this object doesn't support the specified interface, and E_POINTER if
*   ppCP is invalid.
******************************************************************************/
STDMETHODIMP CObjectFrame::FindConnectionPoint( REFIID riid, 
											   IConnectionPoint **ppCP)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::FindConnectionPoint\n");
	
	if (riid == DIID_ISayHelloEvents)
		*ppCP = static_cast<IConnectionPoint*>(this);
	else{
		*ppCP = NULL;
		return CONNECT_E_NOCONNECTION;
	}
	
	static_cast<IUnknown*>(*ppCP)->AddRef();
	return S_OK;
}

/******************************************************************************
*   IConnectionPoint methods -- These methods allow objects to inform this 
*   object that they wish to be informed when events occur, and to check
*   the IID's of outgoing interfaces this object supports.
******************************************************************************/

/******************************************************************************
*   GetConnectionInterface -- This function returns the IID of the outgoing
*   interface that this object supports.  If the address of pIID is not valid
*   this function returns E_POINTER.  If successful, it returns S_OK.
******************************************************************************/
STDMETHODIMP CObjectFrame::GetConnectionInterface( IID *pIID)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetConnectionInterface\n");
	
	HRESULT hr = S_OK;
	if (pIID == NULL)
		hr = E_POINTER;
	else
		*pIID = DIID_ISayHelloEvents;
	
	return hr;
}

/******************************************************************************
*   GetConnectionPointContainer -- This function returns the 
*   IConnectionPointContainer that is the parent of this IConnectionPoint.  If
*   the address of ppCPC is invalid, this function returns E_POINTER.  If
*   successful, it returns S_OK.
******************************************************************************/
STDMETHODIMP CObjectFrame::GetConnectionPointContainer( 
													   IConnectionPointContainer **ppCPC)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::GetConnectionPointContainer\n");
	
	HRESULT hr = S_OK;
	if (ppCPC == NULL)
		hr = E_POINTER;
	else{
		*ppCPC = static_cast<IConnectionPointContainer*>(this);
		static_cast<IUnknown*>(*ppCPC)->AddRef();
	}
	
	return hr;
}

/******************************************************************************
*   Advise -- This function establishes a connection to an event sink that
*   receives events from the outgoing interface that this object supports.
*   It takes an IUnknown* from the sink, and places a cookie that the sink
*   uses to disconnect in pdwCookie.  If the connection is established and 
*   the cookie is placed in pdwCookie, then this function returns S_OK.  If
*   the sink does not recieve events from this object, this function returns
*   CONNECT_E_CANNOTCONNECT.  In addition, this function can return E_POINTER
*   if either parameter is invalid, and CONNECT_E_ADVISELIMIT if this object
*   has reached it's connection limit and cannot accept any more.
******************************************************************************/
STDMETHODIMP CObjectFrame::Advise( IUnknown* pUnk, DWORD *pdwCookie)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Advise\n");
	
	//First check to make sure the parameters are valid
	if (pUnk == NULL)
		return E_POINTER;
	if (pdwCookie == NULL)
		return E_POINTER;
	
	//Make sure there's room for the connection
	if (m_theConnection != NULL)
		return CONNECT_E_ADVISELIMIT;
	
	//Query interface to make sure the sink supports the right interface
	HRESULT hr = pUnk->QueryInterface(DIID_ISayHelloEvents, 
		(void**)&m_theConnection);
	if (FAILED(hr))
		return CONNECT_E_CANNOTCONNECT;
	
	//We got a connection, so set the cookie value
	*pdwCookie = 1;
	
	//ready to send events
	return S_OK;
}

/******************************************************************************
*   Unadvise -- This function uses the value in dwCookie to disconnect from an
*   event sink.  It releases the pointer that was saved in Advise and used to 
*   call sink functions.  This function returns S_OK if the disconnect was 
*   successful, and CONNECT_E_NOCONNECTION if the connection specified by 
*   dwCookie was invalid.
******************************************************************************/
STDMETHODIMP CObjectFrame::Unadvise( DWORD dwCookie)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Unadvise\n");
	
	//Check the value in dwCookie to make sure it's valid
	if (dwCookie != 1)
		return CONNECT_E_NOCONNECTION;
	else{
		m_theConnection->Release();
		m_theConnection = NULL;
		return S_OK;
	}
}

/******************************************************************************
*   EnumConnections -- provides an enumeration of all the sinks that are 
*   currently connected to this object source.
******************************************************************************/
STDMETHODIMP CObjectFrame::EnumConnections( IEnumConnections** ppEnum)
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::EnumConnections\n");
	
	return E_NOTIMPL;
}

/******************************************************************************
*   Class-specific methods -- ObjectFrame is designed to be copied and used as
*   a base for IDispatch-derived classes.  Functions that are not part of the 
*   framework appear below.
******************************************************************************/

/******************************************************************************
*   FireEvent() -- This function calls the MyEvent method defined in the 
*   script, through the engine's IDispatch-like interface.
******************************************************************************/
void CObjectFrame::FireEvent()
{
	//tracing purposed only
	OutputDebugString("CObjectFrame::FireEvent\n");
	
	if (m_theConnection != NULL) {
		DISPPARAMS l_dispParams;
		EXCEPINFO l_exceptions;
		UINT l_error = 0;
		
		l_dispParams.rgvarg            = NULL;
		l_dispParams.rgdispidNamedArgs = NULL;
		l_dispParams.cArgs             = 0;
		l_dispParams.cNamedArgs        = 0;
		
		HRESULT hr = m_theConnection->Invoke( DISPID_ONSONGCHANGE, IID_NULL, 0, 
			DISPATCH_METHOD, &l_dispParams, NULL, &l_exceptions, &l_error);
		if (FAILED(hr))
			OutputDebugString("Error calling event.\n");
	}
	else
		OutputDebugString("Error: no connection.\n");
}

/******************************************************************************
*   In-Script Methods
******************************************************************************/
void CObjectFrame::Play()
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Play\n");
	
	SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_BUTTON_PLAY, 0), 0);
}

void CObjectFrame::Stop( void )
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Stop\n");
	
	SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_BUTTON_STOP, 0), 0);
}

void CObjectFrame::Pause( void )
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Pause\n");
	
	SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_BUTTON_PAUSE, 0), 0);
}

void CObjectFrame::Next( void )
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Next\n");
	
	SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_BUTTON_NEXT, 0), 0);
}

void CObjectFrame::Prev( void )
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::Prev\n");
	
	SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_BUTTON_PREV, 0), 0);
}

void CObjectFrame::AddToLog( OLECHAR *szText )
{
	//tracing purposes only
	OutputDebugString("CObjectFrame::AddToLog\n");
	
	static char sz[256];
	
	// Convert down to ANSI
	WideCharToMultiByte(CP_ACP, 0, szText, -1, sz, 256, NULL, NULL);
	
	if (g_hwndScriptDlg)
		SendMessage(g_hwndScriptDlg, WM_USER, 2, LPARAM(sz));
}

int CObjectFrame::GetPlaylistLength( void )
{
	return SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
}

/***** ISayHello Property Getters *****/
int CObjectFrame::GetVersion( void )
{
	return SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION);
}

int CObjectFrame::GetVolume( void )
{
	return SendMessage(plugin.hwndParent, WM_WA_IPC, -666, IPC_SETVOLUME);
}

int CObjectFrame::GetPanning( void )
{
	return SendMessage(plugin.hwndParent, WM_WA_IPC, -666, IPC_SETPANNING);
}


/***** ISayHello Property Setters *****/
void CObjectFrame::SetVersion( void )
{
	return;
}

void CObjectFrame::SetVolume( int volume )
{
	SendMessage(plugin.hwndParent, WM_WA_IPC, volume, IPC_SETVOLUME);
}

void CObjectFrame::SetPanning( int panning )
{
	SendMessage(plugin.hwndParent, WM_WA_IPC, panning, IPC_SETPANNING);
}

