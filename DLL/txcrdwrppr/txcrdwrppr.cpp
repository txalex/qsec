// txcacwrppr.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include "txcrdwrppr.h"
#include "txmifare.h"

#define BUFFER_SIZE 100

HANDLE	ghStopEvent = NULL;	// global event handle for stopping the thread
HANDLE	ghThread = 0;		// global thread handle
bool	bInserted = false;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

inline LPVOID Allocate(size_t cb)
{
    LPVOID pv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
    if (NULL == pv)
        throw E_OUTOFMEMORY;
    return pv;
}

TXCRDWRPPR_API int TestStrLen(LPTSTR szTest)
{
	int iLen = 0;
	iLen = _tcslen(szTest);
	return iLen;
}


BOOL TXCRDWRPPR_API ExtStartPolling(NATIVE_EVENT CallbackEvent)
{
	PrintDebugInfo(FILE_LOG, L"ExtExtStartPolling()");

	DWORD	dwThreadId = 0;

	//CallbackEvent(1,NULL,0,0);

    // create an event, will be used to kill the polling thread
    if (NULL == (ghStopEvent = CreateEvent(
									 NULL,		// default security attributes
									 TRUE,		// manual reset event
									 FALSE,		// not signaled
									 NULL)))	// no name
    {
		PrintDebugInfo(FILE_LOG, L"ExtExtStartPolling()::CreateEvent() failed: %d", GetLastError());
        return FALSE;
    }

	// start looping thread
	if (NULL == (ghThread = CreateThread( 
								NULL,					// default security attributes 
								0,						// use default stack size  
								CardPolling,			// thread function 
								(LPVOID) CallbackEvent,	// argument to thread function 
								0,						// use default creation flags 
								&dwThreadId)))			// returns the thread identifier 
	{
		PrintDebugInfo(FILE_LOG, L"ExtExtStartPolling()::CreateThread() failed: %d", GetLastError());
		return FALSE;
	}
	
	return TRUE;
}


DWORD TXCRDWRPPR_API ExtGetReaderName(LPTSTR szReader)
{
	int				iLen = 0;
	DWORD			dwRet = 0,
					dwLen = 0;
	SCARDCONTEXT	hCtx = 0;
	LPTSTR			szlReader = L""; 

	if (SCARD_S_SUCCESS != (dwRet = SCardEstablishContext(
										SCARD_SCOPE_SYSTEM, 
										NULL, 
										NULL, 
										&hCtx)))
	{
		PrintDebugInfo(FILE_LOG, L"ExtGetReaderName()::SCardEstablishContext() failed: %X", dwRet);
		return dwRet;
	}

	PrintDebugInfo(FILE_LOG, L"ExtGetReaderName()::SCardEstablishContext() success");

	// check if readers present
	dwLen = SCARD_AUTOALLOCATE;
	if(SCARD_E_NO_READERS_AVAILABLE == (dwRet = SCardListReaders(
													hCtx, 
													NULL, 
													(LPTSTR)(&szlReader), 
													&dwLen)))
	{
		PrintDebugInfo(FILE_LOG, L"ExtGetReaderName()::no readers: %X", dwRet);
		return dwRet;
	}
	else if(SCARD_S_SUCCESS != dwRet)
	{
		PrintDebugInfo(FILE_LOG, L"ExtGetReaderName()::SCardListReaders() failed: %X", dwLen);
		return dwRet;
	}

	PrintDebugInfo(FILE_LOG, L"ExtGetReaderName()::SCardListReaders() success: %s", szlReader);

	iLen = _tcslen(szlReader);

	wcscpy_s(szReader, iLen+1, szlReader);

	PrintDebugInfo(FILE_LOG, L"ExtGetReaderName()::szReader: %s", szReader);

	return SCARD_S_SUCCESS;
}

// thread proc that handles reader and card polling
DWORD WINAPI CardPolling(LPVOID lpParam)
{
    NATIVE_EVENT		callbackEvent = (NATIVE_EVENT)lpParam;
	READER_STATUS*		rdrStReaders = NULL;
    LPTSTR				szReaders = L"", 
						szOldReaders = NULL, 
						szRdr = L"";
    LPSCARD_READERSTATE rgRdrStates = NULL, 
						rgRdrOldStates = NULL, 
						pRdr = NULL;
	DWORD				dwRet = 0, 
						dwLen = 0, 
						dwChanges = 0,
						dw = 0;
	SCARDCONTEXT		hCtx = 0;
    HRESULT				hrSts = 0;
    ULONG				nRdrCount = 0, 
						nRdrOldCount = 0, 
						nIndex = 0,
						nCnt = 0;
	int					iCrdType = INVALID_CARD;
	BYTE*				bpReaders;

	PrintDebugInfo(FILE_LOG, L"CardPolling()");

	try
    {
		hrSts = SCardEstablishContext(
					SCARD_SCOPE_SYSTEM, 
					NULL, 
					NULL, 
					&hCtx );

		if (SCARD_S_SUCCESS != hrSts)
		{
			PrintDebugInfo(FILE_LOG, L"CardPolling()::SCardEstablishContext() failed: %X", hrSts);
			throw hrSts;
		}

		// check if readers present on thread start
		dwLen = SCARD_AUTOALLOCATE;
		if(SCARD_E_NO_READERS_AVAILABLE == (hrSts = SCardListReaders(
														hCtx, 
														NULL, 
														(LPTSTR)(&szReaders), 
														&dwLen)))
		{
			PrintDebugInfo(FILE_LOG, L"CardPolling()::no readers");
			callbackEvent(CLLBK_ERR, NULL, 0, 0);
		}
		else if(SCARD_S_SUCCESS != hrSts)
		{
			PrintDebugInfo(FILE_LOG, L"CardPolling()::SCardListReaders() failed: %X", hrSts);
			throw hrSts;
		}

		szRdr = szReaders;
		nCnt = 0;

		while ('\0' != *szRdr)
		{
			PrintDebugInfo(FILE_LOG, L"CardPolling()::reader[%d]: %s", nCnt, szRdr);
			szRdr += _tcslen(szRdr) + 1;
			nCnt++;
		}

		//PrintDebugInfo(FILE_LOG, L"CardPolling()::SCardListReaders() success: %s, len: %d", szReaders, dwLen);

		// Initialize the PnP detector state.
        nRdrCount = 1;
        rgRdrStates = AllocateStruct(SCARD_READERSTATE);

        rgRdrStates->szReader = TEXT("\\\\?PNP?\\NOTIFICATION");
		//rgRdrStates->szReader = L"OMNIKEY CardMan 5x21 0";
		//rgRdrStates->szReader = szReaders;
		rgRdrStates->dwCurrentState = SCARD_STATE_UNAWARE;

		for (;;)	// reader loop
		{
			for (;;)	// card loop
			{
				//nCnt = 0;
				//for(nCnt = 0; nCnt<nRdrCount; nCnt++)
				//{
				//	PrintDebugInfo(FILE_LOG, L"CardPolling()::rgRdrStates[%d]->szReader = %s", nCnt, rgRdrStates[nCnt].szReader);
				//	PrintDebugInfo(FILE_LOG, L"CardPolling()::rgRdrStates[%d]->dwCurrentState = %X", nCnt, rgRdrStates[nCnt].dwCurrentState);
				//	PrintDebugInfo(FILE_LOG, L"CardPolling()::rgRdrStates[%d]->dwEventState = %X", nCnt, rgRdrStates[nCnt].dwEventState);
				//}

				PrintDebugInfo(FILE_LOG, L"CardPolling()::calling SCardGetStatusChange()");

				hrSts = SCardGetStatusChange(
							hCtx, 
							INFINITE,
							rgRdrStates, 
							nRdrCount);

				if (SCARD_E_CANCELLED == hrSts)
				{
					hrSts = SCARD_S_SUCCESS;
					throw hrSts;
				}
				if (SCARD_S_SUCCESS != hrSts)
					throw hrSts;

				for (nIndex = 1; nIndex < nRdrCount; nIndex += 1)
				{
					PrintDebugInfo(FILE_LOG, L"CardPolling()::internal reader for{}");

					// waiting for SetEvent() to exit thread 
					if(WAIT_OBJECT_0 == (dwRet = WaitForSingleObject(ghStopEvent, 0)))
					{
						PrintDebugInfo(FILE_LOG, L"CardPolling()::WaitForSingleObject()==WAIT_OBJECT_0");
						hrSts = SCARD_S_SUCCESS;
						throw hrSts;
					}
					//else
					//	PrintDebugInfo(FILE_LOG, L"CardPolling()::WaitForSingleObject()=0x%.2X", dwRet);

					//PrintDebugInfo(FILE_LOG, L"CardPolling()::after WaitForSingleObject()");

					pRdr = &rgRdrStates[nIndex];

					// Here we figure out what (if anything) changed on this reader.
					dwChanges = (pRdr->dwEventState ^ pRdr->dwCurrentState) & MAXWORD;
					pRdr->dwCurrentState = pRdr->dwEventState;

					// get reader name length
					dwLen = _tcslen(pRdr->szReader);

					// convert reader name to byte array to pass to gui
					bpReaders = StringToBytPtr((LPTSTR)pRdr->szReader, dwLen);
					
					//PrintDebugInfo(FILE_LOG, L"CardPolling()::after StringToBytPtr()::%s", pRdr->szReader);

					if ((SCARD_STATE_PRESENT & dwChanges & pRdr->dwEventState) &&	// card inserted
					   !(SCARD_STATE_MUTE & pRdr->dwEventState))
					{
						PrintDebugInfo(FILE_LOG, L"CardPolling()::SCARD_STATE_PRESENT::reader=%s", pRdr->szReader);

						// send card inserted to GUI
						//callbackEvent(CLLBK_CARD_INSERTED, L"");

						// use if validating a CAC on card insertion
						TX_PCSC_INFO *pstPCSCInfo = new TX_PCSC_INFO;

						// interrogate card and validate CAC
						// establish new scardcontext and connect to card
						if (SCARD_S_SUCCESS != (dwRet = ActivatePCSC((LPTSTR)pRdr->szReader, SCARD_SHARE_SHARED, pstPCSCInfo)))
						{
							// send error message to GUI
							PrintDebugInfo(FILE_LOG, L"CardPolling()::ActivatePCSC() failed - %X", dwRet);
							callbackEvent(CLLBK_ERR, NULL, 0, 0);
						}
						else
						{
							callbackEvent(CLLBK_CARD_INSERTED, bpReaders, dwLen, iCrdType);

							//// test for CAC/PIV
							//if(SCARD_S_SUCCESS != (dwRet = VerifyGovtCard(pstPCSCInfo, iCrdType)))
							//{
							//	// send not CAC message to GUI
							//	PrintDebugInfo(FILE_LOG, L"CardPolling()::TestForCAC() failed - %X", dwRet);

							//	// send valid CAC inserted to GUI 
							//	callbackEvent(CLLBK_CARD_INVALID, bpReaders, dwLen, 0);
							//}
							//else
							//{
							//	// send valid CAC inserted to GUI 
							//	//MessageBox(NULL,L"CAC",L"",MB_OK);
							//	PrintDebugInfo(FILE_LOG, L"CardPolling()::CLLBK_CAC_PIV_INSERTED");

							//	callbackEvent(CLLBK_CAC_PIV_INSERTED, bpReaders, dwLen, iCrdType);
							//}

							// loop through reader list 
							for(nCnt=0; nCnt<nRdrCount; nCnt++)
							{
								// set an inserted flag on appropriate reader
								if(rdrStReaders[nCnt].szReader == pRdr->szReader)
									rdrStReaders[nCnt].bInserted = true;
							}
						}

						DeactivatePCSC(pstPCSCInfo);
					}
					else if (((SCARD_STATE_EMPTY & dwChanges & pRdr->dwCurrentState) &&		// card removed
							 !(SCARD_STATE_MUTE & pRdr->dwEventState)))	
					{
						// loop through reader list 
						for(nCnt=0; nCnt<nRdrCount-1; nCnt++)
						{
							// check if appropriate reader has a card currently inserted
							if(rdrStReaders[nCnt].szReader == pRdr->szReader && rdrStReaders[nCnt].bInserted == true)
							{
								PrintDebugInfo(FILE_LOG, L"CardPolling()::CLLBK_CARD_REMOVED::reader=%s", pRdr->szReader);

								// set card flag to false
								rdrStReaders[nCnt].bInserted = false;

								// send card removed to GUI
								callbackEvent(CLLBK_CARD_REMOVED, bpReaders, dwLen, 0);	
							}
						}
					}
				}
				pRdr = &rgRdrStates[0];
				pRdr->dwCurrentState = pRdr->dwEventState;
				if (pRdr->dwEventState & SCARD_STATE_CHANGED)
				{
					PrintDebugInfo(FILE_LOG, L"CardPolling()::pRdr->dwEventState & SCARD_STATE_CHANGED::rebuilding reader list");
					break;  // Time to rebuild the reader list
				}
			}

			//
			// Save off the PnP state.
			_ASSERT(NULL == rgRdrOldStates);
			rgRdrOldStates = rgRdrStates;
			nRdrOldCount = nRdrCount;
			_ASSERT(NULL == szOldReaders);
			szOldReaders = szReaders;
			rgRdrStates = NULL;
			nRdrCount = 0;
			szReaders = NULL;
			_ASSERT(NULL == szReaders);
			
			dwLen = SCARD_AUTOALLOCATE;

			// Get a list of readers.
			hrSts = SCardListReaders(hCtx, NULL, (LPTSTR) &szReaders, &dwLen);

			switch (hrSts)
			{
			case SCARD_S_SUCCESS:
			{
				// used for reader list cleanup
				szRdr = szReaders;
				while ('\0' != *szRdr)
				{
					nRdrCount += 1;
					szRdr += _tcslen(szRdr) + 1;
				}

				free(rdrStReaders);

				rdrStReaders = (READER_STATUS*) malloc(nRdrCount * sizeof(READER_STATUS));

				szRdr = szReaders;
				nCnt = 0;

				while ('\0' != *szRdr)
				{
					rdrStReaders[nCnt].szReader = szRdr;
					rdrStReaders[nCnt].bInserted = false;
					szRdr += _tcslen(szRdr) + 1;
					nCnt++;
				}

				// convert wide char reader list string to byte array
				bpReaders = StringToBytPtr(szReaders, dwLen);

				// send reader list to GUI
				callbackEvent(CLLBK_READER_CHANGE, bpReaders, dwLen, 0);
				PrintDebugInfo(FILE_LOG, L"CardPolling()::CLLBK_READER_CHANGE");
				break;
			}
			case SCARD_E_NO_READERS_AVAILABLE:
				// send no reader message to GUI

				PrintDebugInfo(FILE_LOG, L"CardPolling()::CLLBK_READER_CHANGE::NO READERS");
				callbackEvent(CLLBK_READER_CHANGE, NULL, 0, 0);
				break;
			default:
				throw hrSts;
			}

			//
			// Build the new Reader States Array from the old one.
			nRdrCount += 1;
			_ASSERT(NULL == rgRdrStates);
			rgRdrStates = AllocateStructArray(SCARD_READERSTATE, nRdrCount);
			szRdr = szReaders;
			pRdr = rgRdrStates;
			CopyMemory(pRdr, &rgRdrOldStates[0], sizeof(SCARD_READERSTATE));
			while ((NULL != szRdr) && (0 != *szRdr))
			{
				pRdr += 1;
				for (nIndex = 1; nIndex < nRdrOldCount; nIndex += 1)
				{
					if (0 == _tcscmp(szRdr, rgRdrOldStates[nIndex].szReader))
					{
						CopyMemory(pRdr, &rgRdrOldStates[nIndex],
								   sizeof(SCARD_READERSTATE));
						break;
					}
				}
				pRdr->szReader = szRdr;
				szRdr += _tcslen(szRdr) + 1;
			}

			//
			// Clean up for the next pass.
			Free(rgRdrOldStates);
			rgRdrOldStates = NULL;
			nRdrOldCount = 0;
			if (NULL != szOldReaders)
			{
				hrSts = SCardFreeMemory(hCtx, szOldReaders);
				szOldReaders = NULL;
				if (SCARD_S_SUCCESS != hrSts)
					throw hrSts;
			}
		}
    }
    catch (HRESULT hrErr)
    {
        if (NULL != rgRdrStates)
            Free(rgRdrStates);
        if (NULL != szReaders)
            SCardFreeMemory(hCtx, szReaders);
        if (NULL != rgRdrOldStates)
            Free(rgRdrOldStates);
        if (NULL != szOldReaders)
            SCardFreeMemory(hCtx, szOldReaders);

		free(rdrStReaders);

		CloseHandle(ghStopEvent);
		CloseHandle(ghThread);

        return hrErr;
    }
}

BOOL TXCRDWRPPR_API ExtEndPolling()
{
	PrintDebugInfo(FILE_LOG, L"ExtEndPolling()");

	ghThread = NULL;

	// set event to stop looping thread
	return SetEvent(ghStopEvent);
}

TXCRDWRPPR_API void ExtOutputLogging(bool bToggle)
{
	ToggleLogging(bToggle);
}

TXCRDWRPPR_API void ExtSetLogPath(wchar_t* szPath)
{
	SetLogPath(szPath);
	PrintDebugInfo(FILE_LOG, L"ExtSetLogPath()::%s", szPath);
	return;
}


TXCRDWRPPR_API DWORD ExtUpdateSectorTrailers(LPTSTR szReader, BYTE* baCurrentKey, BYTE bAuthKey, BYTE* pbKeyArr, BYTE* baAccessBits)
{
	PrintDebugInfo(FILE_LOG, L"ExtUpdateSectorTrailers()");

	// Structure holding parameters used by the PCSC interface
	TX_PCSC_INFO	*pstPCSCInfo = new TX_PCSC_INFO;
    DWORD	dwRet = 0;
	BYTE	baSectorTrailor[16] = {0},
			baKeyArr[MF_4K_NUM_SECTORS][MF_NUM_KEYS][MF_LEN_KEY] = {};
	int		iBlockNum = 0,
			iIndex = 0,
			iBigSectorCntr = 0;

	try
	{
		// establish new scardcontext and connect to card
		if (SCARD_S_SUCCESS != (dwRet = ActivatePCSC(
											szReader, 
											SCARD_SHARE_SHARED,
											pstPCSCInfo)))
		{
			PrintDebugInfo(FILE_LOG, L"ExtUpdateSectorTrailers()::ActivatePCSC() failed - %X", dwRet);
			throw dwRet;	
		}		
		
		// extract keys from byte pointer
		for (int i = 0; i < MF_4K_NUM_SECTORS; i++)
		{
			for (int j = 0; j < MF_NUM_KEYS; j++)
			{
				for (int k = 0; k < MF_LEN_KEY; k++)
				{
					baKeyArr[i][j][k] = pbKeyArr[iIndex++];
				}
			}
		}

		// load the key to the reader
		if(SCARD_S_SUCCESS != (dwRet = MfLoadKey(pstPCSCInfo, baCurrentKey)))
		{
			PrintDebugInfo(FILE_LOG, L"ExtUpdateSectorTrailers()::MfLoadKey() failed - 0x%.2X", dwRet);
			throw dwRet;
		}

		for (int i = 0; i < MF_4K_NUM_SECTORS; i++)
		{
			// get sector trailer block #
			if(i < 32)
				iBlockNum = (i * MF_1K_NUM_BLOCKS_IN_SECTOR) + MF_NUM_DATA_BLOCKS_LITTLE_SECTOR;
			else
				iBlockNum = (i * MF_1K_NUM_BLOCKS_IN_SECTOR) + MF_NUM_DATA_BLOCKS_BIG_SECTOR + (12 * iBigSectorCntr++);	// still not sure how the '12' adds to the equation, but it works so...

			// authenticate the sector trailer block  
			if(SCARD_S_SUCCESS != (dwRet = MfAuth(pstPCSCInfo, iBlockNum, bAuthKey)))
			{
				PrintDebugInfo(FILE_LOG, L"ExtUpdateSectorTrailers()::MfAuth(%d) failed - 0x%.2X", iBlockNum, dwRet);
				throw dwRet;
			}

			// clear the sector trailer array
			memset(baSectorTrailor, sizeof(baSectorTrailor), 0);

			// copy key A
			memcpy(baSectorTrailor, baKeyArr[i%2], MF_LEN_KEY);

			// copy the access bits
			if(baAccessBits != NULL)
				memcpy(&baSectorTrailor[MF_OFFSET_ACCESS_BITS], baAccessBits, MF_LEN_ACCESS_BITS);
			else
				memcpy(&baSectorTrailor[MF_OFFSET_ACCESS_BITS], MF_DEFAULT_ACCESS_BITS, MF_LEN_ACCESS_BITS);

			// copy key B
			memcpy(&baSectorTrailor[MF_OFFSET_KEY_B], baKeyArr[i%2]+1, MF_LEN_KEY);

			// initialize the sector trailer block of each sector
			if(SCARD_S_SUCCESS != (dwRet = MfWriteBlock(pstPCSCInfo, iBlockNum, baSectorTrailor)))
			{
				PrintDebugInfo(FILE_LOG, L"ExtUpdateSectorTrailers()::MfWrite(block %d) failed - 0x%.2X", iBlockNum, dwRet);
				throw dwRet;
			}			
		}

		throw dwRet;
	}
    catch (DWORD dwErr)
    {
		DeactivatePCSC(pstPCSCInfo);
		PrintDebugInfo(FILE_LOG, L"ExtUpdateSectorTrailers()::end");
		return dwErr;
	}
}

TXCRDWRPPR_API DWORD ExtMfWriteCard(LPTSTR szReader, BYTE* baKey, BYTE bKeyType, BYTE* baData, int iOffset, int iLen)
{
	PrintDebugInfo(FILE_LOG, L"ExtMfWriteCard()");

	TX_PCSC_INFO	*pstPCSCInfo = new TX_PCSC_INFO;
	DWORD dwRet = 0;

	try
	{
		if (SCARD_S_SUCCESS != (dwRet = ActivatePCSC(
											szReader, 
											SCARD_SHARE_SHARED,
											pstPCSCInfo)))
		{
			PrintDebugInfo(FILE_LOG, L"ExtMfWriteCard()::ActivatePCSC() failed - %X", dwRet);
			throw dwRet;	
		}

		if(SCARD_S_SUCCESS != (dwRet = MfReadWriteCard(pstPCSCInfo, baKey, bKeyType, baData, iOffset, iLen, false)))
		{
			PrintDebugInfo(FILE_LOG, L"ExtMfWriteCard()::MfReadWriteCard() failed - 0x%.2X", dwRet);
			throw dwRet;
		}

		throw dwRet;
	}
    catch (DWORD dwErr)
    {
		DeactivatePCSC(pstPCSCInfo);
		PrintDebugInfo(FILE_LOG, L"ExtMfWriteCard()::end");
		return dwErr;
	}
}

TXCRDWRPPR_API DWORD ExtMfReadCard(LPTSTR szReader, BYTE* baKey, BYTE bKeyType, BYTE* baData, int iOffset, int iLen)
{
	PrintDebugInfo(FILE_LOG, L"ExtMfReadCard()");

	TX_PCSC_INFO	*pstPCSCInfo = new TX_PCSC_INFO;
	DWORD dwRet = 0;

	try
	{
		if (SCARD_S_SUCCESS != (dwRet = ActivatePCSC(
											szReader, 
											SCARD_SHARE_SHARED,
											pstPCSCInfo)))
		{
			PrintDebugInfo(FILE_LOG, L"ExtMfReadCard()::ActivatePCSC() failed - %X", dwRet);
			throw dwRet;	
		}

		if(SCARD_S_SUCCESS != (dwRet = MfReadWriteCard(pstPCSCInfo, baKey, bKeyType, baData, iOffset, iLen, true)))
		{
			PrintDebugInfo(FILE_LOG, L"ExtMfReadCard()::MfReadWriteCard() failed - 0x%.2X", dwRet);
			throw dwRet;
		}

		throw dwRet;
	}
    catch (DWORD dwErr)
    {
		DeactivatePCSC(pstPCSCInfo);
		PrintDebugInfo(FILE_LOG, L"ExtMfReadCard()::end");
		return dwErr;
	}
}



TXCRDWRPPR_API DWORD ExtGetCSN(LPTSTR szReader, BYTE* baCSN, int& iLen)
{
	PrintDebugInfo(FILE_LOG, L"ExtGetCSN()");

	TX_PCSC_INFO		*pstPCSCInfo = new TX_PCSC_INFO;
	DWORD				dwRet = 0, dwLen = 0;
	static const BYTE	baGetUID[] = {0xFF, 0xCA, 0x00, 0x00, 0x00};
	unsigned char		ucpRecvBuf[16];

	if (SCARD_S_SUCCESS != (dwRet = ActivatePCSC(
										szReader, 
										SCARD_SHARE_SHARED,
										pstPCSCInfo)))
	{
		PrintDebugInfo(FILE_LOG, L"ExtGetCSN()::ActivatePCSC() failed - %X", dwRet);
		return dwRet;	
	}

	dwLen = LEN_MAX_RESPONSE;

	// get the CSN
	if (SCARD_S_SUCCESS != (dwRet = SCardTransmit(
										pstPCSCInfo->hCard, 
										0, 
										baGetUID, 
										sizeof(baGetUID), 
										NULL, 
										ucpRecvBuf, 
										&dwLen)))
	{
		PrintDebugInfo(FILE_LOG, L"ExtGetCSN()::SCardTransmit(CSN) failed: 0x%.2X", dwRet);
	} 
	else if (STATUS_BYTE_SUCCESS != ucpRecvBuf[dwLen - 2]) // more bytes available
	{
		PrintDebugInfo(FILE_LOG, L"ExtGetCSN()::SCardTransmit(CSN) failed: 0x%.2X%.2X", ucpRecvBuf[dwLen-2], ucpRecvBuf[dwLen-1]);
	}
	else
	{
		iLen = (int) dwLen - 2;
		memcpy(baCSN, ucpRecvBuf, iLen);
	}

	DeactivatePCSC(pstPCSCInfo);
	return dwRet;
}

