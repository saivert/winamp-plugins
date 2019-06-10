/******************************************************************************
*
*   File:   ScriptedFrame.cpp
*
*   Date:   February 19, 1998
*
*   Description:   This file contains the definition of a generic class for 
*               the implementation of an ActiveX Scripting Host.  This 
*               class implements the interfaces necessary to serve as a 
*               Script Host, and can be modified for specific applications 
*               and examples.
*
*   Modifications:
*
******************************************************************************/
#include "stdafx.h"

#include "ScriptedFrame.h"
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>

extern HWND g_hwndScriptDlg;
extern char g_scriptlang[256];

void ShowBSTR(char *prompt, BSTR pbStr) {
	static char sz[256];
	static char txt[8192];
	
	// Convert down to ANSI
	WideCharToMultiByte(CP_ACP, 0, pbStr, -1, sz, 256, NULL, NULL);
	
	sprintf(txt, "%s: %s", prompt, sz);
	//::MessageBox(NULL, txt, "ShowBSTR", MB_SETFOREGROUND | MB_OK);
	if (g_hwndScriptDlg)
		::SendMessage(g_hwndScriptDlg, WM_USER, 2, LPARAM(sz));
}

char* CheckRegKey(char* regPath, char*keyName)
{
	HKEY hKey;					//Handle to the key being opened
	DWORD retCode;				//Value for return codes
	DWORD ValueName = 255;		//
	DWORD BufferLength = 255;	//Length of buffer containing the argument
	char* theValue = "";
	
	//Open the registry key
	retCode = RegOpenKeyEx(HKEY_CURRENT_USER,
		regPath, NULL, KEY_ALL_ACCESS, &hKey);
	
	//Check for an error
	if (retCode != ERROR_SUCCESS){
		// check to see what the error condition was
		if (retCode == ERROR_ACCESS_DENIED)
			// Access Denied...
			OutputDebugString ("\nError: unable to open key.  Probably due to "
			"security reasons.");
		else
			// if it was not Access Denied then just report what we know
			OutputDebugString ("Error: Unable to open key");
	}
	else{
		//Check the value
		retCode = RegQueryValueEx( hKey, keyName, NULL, NULL, (UCHAR*)theValue, 
			&BufferLength);
		
		RegCloseKey(hKey);	// Key handle returned from RegOpenKeyEx.
		
		// check to see if we had an error
		if (retCode != ERROR_SUCCESS)
		{
			// check to see what the error condition was
			if (retCode == ERROR_ACCESS_DENIED)
				// Access Denied...
				OutputDebugString ("\nError: unable to open key.  Probably due" 
				"to security reasons.");
			else
				// if it was not Access Denied then just report what we know
				OutputDebugString ("Error: Unable to open key");
		}
	}
	
	return theValue;
}

//Constructor
CScriptedFrame::CScriptedFrame()
{
	m_refCount = 0;
	m_Engine = NULL;

	m_Object = NULL;
	OutputDebugString("CScriptedFrame\n");
}

//Destructor
CScriptedFrame::~CScriptedFrame()
{
	OutputDebugString("~CScriptedFrame\n");
	
	if (m_Engine != NULL){
		m_Engine->Release();
		m_Engine = NULL;
	}
	if (m_Parser != NULL){
		m_Parser->Release();
		m_Parser = NULL;
	}
	if (m_Object != NULL){
		m_Object->Release();
		m_Object = NULL;
	}
}

bool CScriptedFrame::SetScript(LPCTSTR szScript)
{
	//Copy the file to the script buffer, which is a Unicode string
	if (!MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szScript, -1, theScript, 8192)){
		OutputDebugString("Error translating string.\n");
		return false;
	}
	
	return true;
}

bool CScriptedFrame::IsActive()
{
	SCRIPTSTATE ss;
	HRESULT hr;
	if (m_Engine) {
		hr = m_Engine->GetScriptState(&ss);
		return (ss == SCRIPTSTATE_CONNECTED);
	}
	return false;
}

/******************************************************************************
*   InitializeScriptFrame -- Creates the ActiveX Scripting Engine and 
*   initializes it.  Returns true if successful, false otherwise.
******************************************************************************/
BOOL CScriptedFrame::InitializeScriptFrame(HWND hwndParent)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::InitializeScriptedFrame\n");
	
	m_hwndParent = hwndParent;

	if (g_scriptlang[0])
	{
		WCHAR wsz[256];
		if (!MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, g_scriptlang, -1, wsz, 256)){
			OutputDebugString("Error translating string. (CObjectFrame::InitializeScriptFrame)\n");
		}
		CLSIDFromProgID(wsz, &m_EngineClsid);
	} else {
		CLSIDFromProgID(L"VBScript", &m_EngineClsid);
	}

	
	//First, create the scripting engine with a call to CoCreateInstance, 
	//placing the created engine in m_Engine.
	HRESULT hr = CoCreateInstance(m_EngineClsid, NULL, CLSCTX_INPROC_SERVER, 
		IID_IActiveScript, (void **)&m_Engine);
	if (FAILED(hr))
	{
		OutputDebugString("Failed to create scripting engine.\n");
		return false;
	}
	
	//Now query for the IActiveScriptParse interface of the engine
	hr = m_Engine->QueryInterface(IID_IActiveScriptParse, (void**)&m_Parser);
	if (FAILED(hr))
	{
		OutputDebugString("Engine doesn't support IActiveScriptParse.\n");
		return false;
	}
	
	//The engine needs to know the host it runs on.
	hr = m_Engine->SetScriptSite(this);
	if (FAILED(hr))
	{
		OutputDebugString("Error calling SetScriptSite\n");
		return false;
	}
	
	//Initialize the script engine so it's ready to run.
	hr = m_Parser->InitNew();
	if (FAILED(hr))
	{
		OutputDebugString("Error calling InitNew\n");
		return false;
	}
	
	//Open and read the file of the script to be run
	/*
	ifstream scriptStream("MyScript.txt", ios::binary | ios::in);
	if (!scriptStream) {
		OutputDebugString("Error opening script file.\n");
		return false;
	}
	   
	//Read the file into a string
	char str[8192];
	scriptStream.read(str, 8192);
	
	//Copy the file to the script buffer, which is a Unicode string
	if (!MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, str, -1, theScript, 8192)) {
		OutputDebugString("Error translating string.\n");
		return false;
	}
	*/
	
	//everything succeeded.
	return true;
	
}

/******************************************************************************
*   RunScript -- starts the script engine and executes it's instructions.
******************************************************************************/
void CScriptedFrame::RunScript()
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::RunScript\n");
	
	//Create an object for the script to interact with.
	m_Object = new CObjectFrame();
	m_Object->AddRef();
	
	//Add the name of the object that will respond to the script
	m_Engine->AddNamedItem(L"Winamp", SCRIPTITEM_ISSOURCE | 
		SCRIPTITEM_ISVISIBLE);
	
	//Pass the script to be run to the script engine with a call to 
	//ParseScriptText
	HRESULT hr = m_Parser->ParseScriptText(theScript, L"Winamp", NULL, NULL, 
		0, 0, 0L, NULL, NULL);
	if (FAILED(hr))
		OutputDebugString("Error calling ParseScriptText\n");
	
	//Tell the engine to start processing the script with a call to 
	//SetScriptState().
	hr = m_Engine->SetScriptState(SCRIPTSTATE_CONNECTED);
	if (FAILED(hr))
		OutputDebugString("Error calling SetScriptState\n");
}

/******************************************************************************
*	QuitScript -- Closes the script engine and cleans up.
******************************************************************************/
void CScriptedFrame::QuitScript(bool bFullyTerminate)
{
	if (bFullyTerminate) {
		HRESULT hr = m_Engine->Close();
		if (FAILED(hr))
			OutputDebugString("Error calling Close\n");
	} else {
		HRESULT hr = m_Engine->SetScriptState(SCRIPTSTATE_DISCONNECTED);
		if (FAILED(hr))
			OutputDebugString("Error calling SetScriptState\n");
	}
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
STDMETHODIMP CScriptedFrame::QueryInterface(REFIID riid, void ** ppvObj)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::QueryInterface->");
	
	if (riid == IID_IUnknown){
		OutputDebugString("IUnknown\n");
		*ppvObj = static_cast<IActiveScriptSite*>(this);
	}
	
	else if (riid == IID_IActiveScriptSite){
		OutputDebugString("IActiveScriptSite\n");
		*ppvObj = static_cast<IActiveScriptSite*>(this);
	}
	
	else if (riid == IID_IActiveScriptSiteWindow){
		OutputDebugString("IActiveScriptSiteWindow\n");
		*ppvObj = static_cast<IActiveScriptSiteWindow*>(this);
	}
	
	else{
		OutputDebugString("Unsupported Interface  \n");
		
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
STDMETHODIMP_(ULONG) CScriptedFrame::AddRef()
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::AddRef\n");
	
	return ++m_refCount;
}

/******************************************************************************
*   Release() -- When a reference to this object is removed, this function 
*   decrements the reference count.  If the reference count is 0, then this 
*   function deletes this object and returns 0;
******************************************************************************/
STDMETHODIMP_(ULONG) CScriptedFrame::Release()
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::Release\n");
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
*   IActiveScriptSite Interfaces -- These interfaces define the exposed methods
*   of ActiveX Script Hosts.
******************************************************************************/

/******************************************************************************
*   GetLCID() -- Gets the identifier of the host's user interface.  This method 
*   returns S_OK if the identifier was placed in plcid, E_NOTIMPL if this 
*   function is not implemented, in which case the system-defined identifier
*   should be used, and E_POINTER if the specified pointer was invalid.
******************************************************************************/
STDMETHODIMP CScriptedFrame::GetLCID( LCID *plcid )
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::GetLCID\n");
	
	return E_NOTIMPL;
}

/******************************************************************************
*   GetItemInfo() -- Retrieves information about an item that was added to the 
*   script engine through a call to AddNamedItem.
*   Parameters:   pstrName -- the name of the item, specified in AddNamedItem.
*            dwReturnMask -- Mask indicating what kind of pointer to return
*               SCRIPTINFO_IUNKNOWN or SCRIPTINFO_ITYPEINFO
*            ppunkItem -- return spot for an IUnknown pointer
*            ppTypeInfo -- return spot for an ITypeInfo pointer
*   Returns:   S_OK if the call was successful
*            E_INVALIDARG if one of the arguments was invalid
*            E_POINTER if one of the pointers was invalid
*            TYPE_E_ELEMENTNOTFOUND if there wasn't an item of the 
*               specified type.
******************************************************************************/
STDMETHODIMP CScriptedFrame::GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask,
										 IUnknown **ppunkItem, ITypeInfo **ppTypeInfo)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::GetItemInfo\n");
	
	//Use logical ANDs to determine which type(s) of pointer the caller wants, 
	//and make sure that that placeholder is currently valid.
	if (dwReturnMask & SCRIPTINFO_IUNKNOWN){
		if (!ppunkItem)
			return E_INVALIDARG;
		*ppunkItem = NULL;
	}
	if (dwReturnMask & SCRIPTINFO_ITYPEINFO){
		if (!ppTypeInfo)
			return E_INVALIDARG;
		*ppTypeInfo = NULL;
	}
	
	/****** Do tests for named items here.  *******/
	if (!_wcsicmp(L"Winamp", pstrName)){
		if (dwReturnMask & SCRIPTINFO_IUNKNOWN){
			m_Object->QueryInterface(IID_IUnknown, (void**)ppunkItem);
			return S_OK;
		}
		else if (dwReturnMask & SCRIPTINFO_ITYPEINFO){
			return m_Object->LoadTypeInfo(ppTypeInfo, CLSID_ashObject, 0);
		}
	}
	
	return TYPE_E_ELEMENTNOTFOUND;
}

/******************************************************************************
*   GetDocVersionString() -- It is possible, even likely that a script document
*   can be changed between runs.  The host can define a unique version number 
*   for the script, which can be saved along with the script.  If the version 
*   changes, the engine will know to recompile the script on the next run.
******************************************************************************/
STDMETHODIMP CScriptedFrame::GetDocVersionString(BSTR *pbstrVersionString)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::GetDocVersionString\n");
	
	//For the generic case, this function isn't implemented.
	return E_NOTIMPL;
}

/******************************************************************************
*   OnScriptTerminate() -- This method may give the host a chance to react when
*   the script terminates.  pvarResult give the result of the script or NULL
*   if the script doesn't give a result, and pexcepinfo gives the location of
*   any exceptions raised by the script.  Returns S_OK if the calls succeeds.
******************************************************************************/
STDMETHODIMP CScriptedFrame::OnScriptTerminate(const VARIANT *pvarResult, 
											   const EXCEPINFO *pexcepinfo)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::OnScriptTerminate\n");
	
	//If something needs to happen when the script terminates, put it here.
	return S_OK;
}

/******************************************************************************
*   OnStateChange() -- This function gives the host a chance to react when the
*   state of the script engine changes.  ssScriptState lets the host know the
*   new state of the machine.  Returns S_OK if successful.
******************************************************************************/
STDMETHODIMP CScriptedFrame::OnStateChange( SCRIPTSTATE ssScriptState)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::OnStateChange\n");
	
	//If something needs to happen when the script enters a certain state, 
	//put it here.
	switch (ssScriptState){
	case SCRIPTSTATE_UNINITIALIZED:
		OutputDebugString("State: Uninitialized.\n");
		break;
	case SCRIPTSTATE_INITIALIZED:
		OutputDebugString("State: Initialized.\n");
		break;
	case SCRIPTSTATE_STARTED:
		OutputDebugString("State: Started.\n");
		break;
	case SCRIPTSTATE_CONNECTED:
		OutputDebugString("State: Connected.\n");
		break;
	case SCRIPTSTATE_DISCONNECTED:
		OutputDebugString("State: Disconnected.\n");
		break;
	case SCRIPTSTATE_CLOSED:
		OutputDebugString("State: Closed.\n");
		break;
	default:
		break;
	}
	
	return S_OK;
}

/******************************************************************************
*   OnScriptError() -- This function gives the host a chance to respond when 
*   an error occurs while running a script.  pase holds a reference to the 
*   IActiveScriptError object, which the host can use to get information about
*   the error.  Returns S_OK if the error was handled successfully, and an OLE
*   error code if not.
******************************************************************************/
STDMETHODIMP CScriptedFrame::OnScriptError(IActiveScriptError *pase)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::OnScriptError\n");
	
	//Display a message box with information about the error.
	EXCEPINFO theException;
	HRESULT hr = pase->GetExceptionInfo(&theException);
	if (SUCCEEDED(hr)) {
		ShowBSTR("Script Error:", theException.bstrDescription);
	}
	
	return S_OK;
}

/******************************************************************************
*   OnEnterScript() -- This function gives the host a chance to respond when
*   the script begins running.  Returns S_OK if the call was successful.
******************************************************************************/
STDMETHODIMP CScriptedFrame::OnEnterScript(void)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::OnEnterScript\n");
	
    ShowBSTR("Status: ", L"Status: Entering script");

	return S_OK;
}

/******************************************************************************
*   OnExitScript() -- This function gives the host a chance to respond when
*   the script finishes running.  Returns S_OK if the call was successful.
******************************************************************************/
STDMETHODIMP CScriptedFrame::OnLeaveScript(void)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::OnLeaveScript\n");

    ShowBSTR("Status: ", L"Status: Leaving script");
	
	return S_OK;
}

/******************************************************************************
*   IActiveScriptSiteWindow -- This interface allows the script engine to 
*   manipulate the user interface, if it's located in the same object as the 
*   IActiveScriptSite.
******************************************************************************/

/******************************************************************************
*   GetWindow() -- This function returns a handle to a window that the script
*   engine can use to display information to the user.  Returns S_OK if the 
*   call was successful, and E_FAIL if there was an error.
******************************************************************************/
STDMETHODIMP CScriptedFrame::GetWindow(HWND *phwnd)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::GetWindow\n");
	
	//If there is a window that the script engine can use, pass it back.
	//Otherwise, this function should be removed.
	*phwnd = m_hwndParent;
	return S_OK;
}

/******************************************************************************
*   EnableModeless() -- This function instructs the host to enable or disable
*   it's main window and any modeless dialog boxes it may have.  Returns S_OK
*   if successful, and E_FAIL if not.
******************************************************************************/
STDMETHODIMP CScriptedFrame::EnableModeless(BOOL fEnable)
{
	//tracing purposes only
	OutputDebugString("CScriptedFrame::EnableModeless\n");
	
	//Do any enabling or disabling required.
	return E_FAIL;
}

/******************************************************************************
*   FireEvent() -- This function calls the FireEvent method of m_Object and 
*   then cleans up the script engine, in preparation for closing the 
*   application.
******************************************************************************/
void CScriptedFrame::FireEvent()
{
	m_Object->FireEvent();
}
