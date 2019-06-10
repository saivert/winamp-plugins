/******************************************************************************
*
*   File:   ObjectFrame.h
*
*   Date:   March 3, 1998
*
*   Description:   This file contains the declaration of a generic class that
*               implements the IDispatch interface, which allows the 
*               methods of this class to be called by anyone who understands
*               the IDispatch interface.
*
*   Modifications:
******************************************************************************/
#ifndef Object_Frame_H
#define Object_Frame_H

#include "multinfo.h"
#include "ashhelloworld.h"

class CObjectFrame : public ISayHello, public IProvideMultipleClassInfo, 
   public IConnectionPointContainer, public IConnectionPoint{
protected:
   int m_refCount;
   ITypeInfo* m_typeInfo;
   ISayHelloEvents* m_theConnection;

public:
   //Constructor
   CObjectFrame();
   //Destructor
   ~CObjectFrame();

   void FireEvent();

   /***** Type Library Methods *****/
   STDMETHODIMP LoadTypeInfo(ITypeInfo** pptinfo, REFCLSID clsid, LCID lcid);

   /***** IUnknown Methods *****/
   STDMETHODIMP QueryInterface(REFIID riid, void**ppvObj);
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
 
   /***** IDispatch Methods *****/
   STDMETHODIMP GetTypeInfoCount(UINT* iTInfo);
   STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
   STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames,
      UINT cNames, LCID lcid, DISPID* rgDispId);
   STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,  
      WORD wFlags, DISPPARAMS* pDispParams,  VARIANT* pVarResult,
      EXCEPINFO* pExcepInfo,  UINT* puArgErr);

   /***** IProvideClassInfo Methods *****/
   STDMETHODIMP GetClassInfo( ITypeInfo** ppTI );

   /***** IProvideClassInfo2 Methods *****/
   STDMETHODIMP GetGUID( DWORD dwGuidKind, GUID* pGUID);

   /***** IProvideMultipleClassInfo Methods *****/
   STDMETHODIMP GetMultiTypeInfoCount( ULONG* pcti);
   STDMETHODIMP GetInfoOfIndex( ULONG iti, DWORD dwMCIFlags, 
      ITypeInfo **pptiCoClass, DWORD *pdwTIFlags, ULONG *pcdispidReserved,
      IID *piidPrimary, IID *piidSource);

   /***** IConnectionPointContainer Methods *****/
   STDMETHODIMP EnumConnectionPoints( IEnumConnectionPoints **ppEnum);
   STDMETHODIMP FindConnectionPoint( REFIID riid, IConnectionPoint **ppCP);

   /***** IConnectionPoint Methods *****/
   STDMETHODIMP GetConnectionInterface( IID *pIID);
   STDMETHODIMP GetConnectionPointContainer( IConnectionPointContainer **ppCPC);
   STDMETHODIMP Advise( IUnknown* pUnk, DWORD *pdwCookie);
   STDMETHODIMP Unadvise( DWORD dwCookie);
   STDMETHODIMP EnumConnections( IEnumConnections** ppEnum);

   /***** ISayHello Property Getters *****/
   int GetVersion( void );
   int GetVolume( void );
   int GetPanning( void );

   /***** ISayHello Property Setters *****/
   void SetVersion( void );
   void SetVolume( int );
   void SetPanning( int );

   /***** ISayHello Methods *****/
  void Play( void );
  void Stop( void );
  void Pause( void );
  void Next( void );
  void Prev( void );
  void AddToLog( BSTR szText );
  int GetPlaylistLength( void );
};
#endif
