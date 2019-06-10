/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Mar 09 12:01:14 1998
 */
/* Compiler settings for ashhelloworld.odl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __ashhelloworld_h__
#define __ashhelloworld_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISayHello_FWD_DEFINED__
#define __ISayHello_FWD_DEFINED__
typedef interface ISayHello ISayHello;
#endif 	/* __ISayHello_FWD_DEFINED__ */


#ifndef __ISayHelloEvents_FWD_DEFINED__
#define __ISayHelloEvents_FWD_DEFINED__
typedef interface ISayHelloEvents ISayHelloEvents;
#endif 	/* __ISayHelloEvents_FWD_DEFINED__ */


#ifndef __ashObject_FWD_DEFINED__
#define __ashObject_FWD_DEFINED__

#ifdef __cplusplus
typedef class ashObject ashObject;
#else
typedef struct ashObject ashObject;
#endif /* __cplusplus */

#endif 	/* __ashObject_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __ashHelloWorld_LIBRARY_DEFINED__
#define __ashHelloWorld_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ashHelloWorld
 * at Mon Mar 09 12:01:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [version][uuid] */ 



EXTERN_C const IID LIBID_ashHelloWorld;

#ifndef __ISayHello_DISPINTERFACE_DEFINED__
#define __ISayHello_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: ISayHello
 * at Mon Mar 09 12:01:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid] */ 



EXTERN_C const IID DIID_ISayHello;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("156EE0C1-B426-11d1-94A1-006008939020")
    ISayHello : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct ISayHelloVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISayHello __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISayHello __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISayHello __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISayHello __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISayHello __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISayHello __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISayHello __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } ISayHelloVtbl;

    interface ISayHello
    {
        CONST_VTBL struct ISayHelloVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISayHello_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISayHello_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISayHello_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISayHello_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISayHello_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISayHello_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISayHello_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __ISayHello_DISPINTERFACE_DEFINED__ */


#ifndef __ISayHelloEvents_DISPINTERFACE_DEFINED__
#define __ISayHelloEvents_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: ISayHelloEvents
 * at Mon Mar 09 12:01:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid] */ 



EXTERN_C const IID DIID_ISayHelloEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("A5673BE0-B512-11d1-94A1-006008939020")
    ISayHelloEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct ISayHelloEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISayHelloEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISayHelloEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISayHelloEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISayHelloEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISayHelloEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISayHelloEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISayHelloEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } ISayHelloEventsVtbl;

    interface ISayHelloEvents
    {
        CONST_VTBL struct ISayHelloEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISayHelloEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISayHelloEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISayHelloEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISayHelloEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISayHelloEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISayHelloEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISayHelloEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __ISayHelloEvents_DISPINTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_ashObject;

class DECLSPEC_UUID("854C92C1-B426-11d1-94A1-006008939020")
ashObject;
#endif
#endif /* __ashHelloWorld_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
