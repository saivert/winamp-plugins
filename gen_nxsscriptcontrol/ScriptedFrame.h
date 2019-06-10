/******************************************************************************
*
*   File:   ScriptedFrame.h
*
*   Date:   February 19, 1998
*
*   Description:   This file contains the declaration of a generic class for 
*               the implementation of an ActiveX Scripting Host.  This 
*               class implements the interfaces necessary to serve as a 
*               Script Host, and can be modified for specific applications 
*               and examples.
*
*   Modifications:
*
******************************************************************************/
#include "ObjectFrame.h"
#include "activscp.h"

class CScriptedFrame :   public IActiveScriptSite,
                  public IActiveScriptSiteWindow
{
protected:
   int m_refCount;               //variable to maintain the reference count
   IActiveScript* m_Engine;      //reference to the scripting engine
   IActiveScriptParse* m_Parser;   //reference to scripting engine in parsing 
                           //mode.
   CLSID m_EngineClsid;         //The CLSID of the script engine we want 
                           //to use.
   CObjectFrame* m_Object;         //The object that will interact with the 
                           //script
   WCHAR theScript[8192];	//The script that is being run.
   HWND m_hwndParent;

public:
   //Constructor
   CScriptedFrame();
   //Destructor
   ~CScriptedFrame();

   BOOL InitializeScriptFrame(HWND hwndParent);
   void RunScript();
   void QuitScript(bool bFullyTerminate);
   void FireEvent();
   bool SetScript(LPCTSTR szScript);
   bool IsActive();

   /******* IUnknown *******/
   STDMETHODIMP QueryInterface(REFIID riid, void * * ppvObj);
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();

   /******* IActiveScriptSite *******/
   STDMETHODIMP GetLCID( LCID *plcid );   // address of variable for 
                                 //language identifier
   STDMETHODIMP GetItemInfo(
      LPCOLESTR pstrName,          // address of item name
      DWORD dwReturnMask,         // bit mask for information retrieval
      IUnknown **ppunkItem,      // address of pointer to item's IUnknown
      ITypeInfo **ppTypeInfo);   // address of pointer to item's ITypeInfo
   STDMETHODIMP GetDocVersionString(
      BSTR *pbstrVersionString);  // address of document version string
   STDMETHODIMP OnScriptTerminate(
      const VARIANT *pvarResult,   // address of script results
      const EXCEPINFO *pexcepinfo);   // address of structure with exception 
                              //information
   STDMETHODIMP OnStateChange(
      SCRIPTSTATE ssScriptState);   // new state of engine
   STDMETHODIMP OnScriptError(
      IActiveScriptError *pase);   // address of error interface
   STDMETHODIMP OnEnterScript(void);
   STDMETHODIMP OnLeaveScript(void);

   /******* IActiveScriptSiteWindow *******/
   STDMETHODIMP GetWindow(HWND *phwnd);
   STDMETHODIMP EnableModeless(BOOL fEnable);  // enable flag

};
