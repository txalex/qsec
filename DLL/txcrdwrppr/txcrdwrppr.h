// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the TXCACWRPPR_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// TXCRDWRPPR_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#include "txhelperslib.h"
//#include "txmifare.h"


#ifdef TXCRDWRPPR_EXPORTS
#define TXCRDWRPPR_API __declspec(dllexport)
#else
#define TXCRDWRPPR_API __declspec(dllimport)
#endif

#define READER_NAME L"OMNIKEY CardMan 5x21 0"

DWORD WINAPI CardPolling(LPVOID lpParam);

// Type definition for the callback function.
typedef void (CALLBACK *NATIVE_EVENT)(
							int,		// event
							BYTE*,		// reader list
							DWORD,		// reader list length
							int);		// card type if applicable

//typedef void (CALLBACK *NATIVE_EVENT)();

extern "C" 
{
	// generic functions	
	TXCRDWRPPR_API DWORD	ExtGetCSN(LPTSTR szReader, BYTE* baCSN, int& iLen);

	// polling functions
	TXCRDWRPPR_API BOOL		ExtStartPolling(NATIVE_EVENT CallbackEvent);	// start polling
	TXCRDWRPPR_API BOOL		ExtEndPolling();	// end polling

	// debug output functions
	TXCRDWRPPR_API void		ExtOutputLogging(bool bToggle);
	TXCRDWRPPR_API void		ExtSetLogPath(wchar_t* szPath);

	// mifare functions
	TXCRDWRPPR_API DWORD	ExtMfWriteCard(LPTSTR szReader, BYTE* baKey, BYTE bKeyType, BYTE* baData, int iOffset, int iLen);
	TXCRDWRPPR_API DWORD	ExtMfReadCard(LPTSTR szReader, BYTE* baKey, BYTE bKeyType, BYTE* baData, int iOffset, int iLen);
	TXCRDWRPPR_API DWORD	ExtUpdateSectorTrailers(LPTSTR szReader, BYTE* baCurrentKey, BYTE bAuthKey, BYTE* pbKeyArr, BYTE* baAccessBits);
}

