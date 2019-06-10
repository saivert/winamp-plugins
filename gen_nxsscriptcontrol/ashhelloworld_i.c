/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Mar 09 12:01:14 1998
 */
/* Compiler settings for ashhelloworld.odl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID LIBID_ashHelloWorld = {0xC3079A20,0xB425,0x11d1,{0x94,0xA1,0x00,0x60,0x08,0x93,0x90,0x20}};


const IID DIID_ISayHello = {0x156EE0C1,0xB426,0x11d1,{0x94,0xA1,0x00,0x60,0x08,0x93,0x90,0x20}};


const IID DIID_ISayHelloEvents = {0xA5673BE0,0xB512,0x11d1,{0x94,0xA1,0x00,0x60,0x08,0x93,0x90,0x20}};


const CLSID CLSID_ashObject = {0x854C92C1,0xB426,0x11d1,{0x94,0xA1,0x00,0x60,0x08,0x93,0x90,0x20}};


#ifdef __cplusplus
}
#endif

