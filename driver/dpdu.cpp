#include "pch.h"
#include "dpdu.h"
#include "Logger.h"
#include "pdu_api.h"
#include "j2534/shim_loader.h"
#include "ComLogicalLink.h"

#include <chrono>
#include <vector>
#include <string>
#include <map>

static std::vector<cPassThruInfo> m_registryList;
static unsigned long m_deviceID = 0;
static std::map<UNUM32, std::string>m_objectIdMap;
static std::map<UNUM32, std::shared_ptr<ComLogicalLink>> m_commChannels;

T_PDU_ERROR __stdcall PDUConstruct(CHAR8* OptionStr, void* pAPITag)
{
	LOGGER.logWarn("STUB", "PDUConstruct is unimplimented");
	if (OptionStr != nullptr) {
		LOGGER.logWarn("STUB", "OptionStr: %s", OptionStr);
	}

	LOGGER.logWarn("STUB", "PDUConstruct return");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUDestruct()
{
	LOGGER.logWarn("STUB", "PDUDestruct is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUModuleConnect(UNUM32 hMod)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;

	LOGGER.logWarn("PDUModuleConnect", "Connecting to module: %u, library: %s", hMod, m_registryList[hMod].FunctionLibrary.c_str());

	bool sret = shim_loadLibrary(m_registryList[hMod].FunctionLibrary.c_str());
	if (!sret)
	{
		LOGGER.logWarn("PDUModuleConnect", "shim_loadLibrary failed");
		ret = PDU_ERR_MODULE_NOT_CONNECTED;
	}

	if (ret == PDU_STATUS_NOERROR)
	{
		long ptret = _PassThruOpen(NULL, &m_deviceID);
		if (ptret != STATUS_NOERROR)
		{
			LOGGER.logWarn("PDUModuleConnect", "_PassThruOpen failed: %d", ptret);
			ret = PDU_ERR_MODULE_NOT_CONNECTED;
		}
	}

	return ret;
}

T_PDU_ERROR __stdcall PDUModuleDisconnect(UNUM32 hMod)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;

	LOGGER.logInfo("PDUModuleDisconnect", "hMod %u", hMod);

	long ptret = _PassThruClose(m_deviceID);
	if (ptret != STATUS_NOERROR)
	{
		LOGGER.logError("PDUModuleDisconnect", "_PassThruClose failed: %d", ptret);
		ret = PDU_ERR_MODULE_NOT_CONNECTED;
	}

	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetTimestamp(UNUM32 hMod, UNUM32* pTimestamp)
{
	LOGGER.logWarn("STUB", "PDUGetTimestamp is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUIoCtl(UNUM32 hMod, UNUM32 hCLL, UNUM32 IoCtlCommandId, PDU_DATA_ITEM* pInputData, PDU_DATA_ITEM** pOutputData)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;
	long cllret = STATUS_NOERROR;

	auto itCLL = m_commChannels.find(hCLL);

	auto it = m_objectIdMap.find(IoCtlCommandId);
	if (it != m_objectIdMap.end())
	{
		if (it->second == std::string("PDU_IOCTL_READ_VBATT"))
		{
			unsigned long volt;

			long ptret = _PassThruIoctl(m_deviceID, READ_VBATT, NULL, &volt);
			if (ptret != STATUS_NOERROR)
			{
				LOGGER.logWarn("PDUIoCtl", "_PassThruIoctl failed: %d", ptret);
				ret = PDU_ERR_CABLE_UNKNOWN;
			}

			LOGGER.logInfo("PDUIoCtl", "VBATT %u", volt);

			*pOutputData = new PDU_DATA_ITEM;
			(*pOutputData)->ItemType = PDU_IT_IO_UNUM32;
			(*pOutputData)->pData = new UNUM32;
			*(UNUM32*)((*pOutputData)->pData) = volt;
		}
		else if (it->second == std::string("PDU_IOCTL_CLEAR_MSG_FILTER"))
		{
		}
		else if (it->second == std::string("PDU_IOCTL_START_MSG_FILTER"))
		{
			LOGGER.logInfo("PDUIoCtl", "PDU_IOCTL_START_MSG_FILTER ItemType 0x%x, Data 0x%x",
				pInputData->ItemType, *(UNUM32*)(pInputData->pData));

			if (itCLL != m_commChannels.end())
			{
				cllret = itCLL->second->StartMsgFilter(*(unsigned long*)(pInputData->pData));

				if (cllret != STATUS_NOERROR)
				{
					LOGGER.logWarn("PDUIoCtl", "_PassThruStartMsgFilter failed: %d", cllret);
					ret = PDU_ERR_CABLE_UNKNOWN;
				}
			}
		}
	}

	LOGGER.logInfo("PDUIoCtl", "hMod %u, hCLL %u, IoCtlCommandId %u, pInputData %p, pOutputData %p", hMod, hCLL, IoCtlCommandId, pInputData, pOutputData);

	return ret;
}

T_PDU_ERROR __stdcall PDUGetVersion(UNUM32 hMod, PDU_VERSION_DATA* pVersionData)
{
	LOGGER.logWarn("STUB", "PDUGetVersion is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetStatus(UNUM32 hMod, UNUM32 hCLL, UNUM32 hCoP, T_PDU_STATUS* pStatusCode, UNUM32* pTimestamp, UNUM32* pExtraInfo)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;
	T_PDU_STATUS status = PDU_MODST_NOT_AVAIL;

	auto it = m_commChannels.find(hCLL);

	if (hCLL == PDU_HANDLE_UNDEF && hCoP == PDU_HANDLE_UNDEF)
	{
		//TODO get actual module status
		status = PDU_MODST_READY;
	}
	else if (hCoP == PDU_HANDLE_UNDEF)
	{
		if (it != m_commChannels.end())
		{
			ret = it->second->GetStatus(status);
		}
	}
	else
	{
		if (it != m_commChannels.end())
		{
			ret = it->second->GetStatus(hCoP, status);
		}
	}

	*pStatusCode = status;

	if (pExtraInfo != nullptr) {
		*pExtraInfo = 0;
	}

	LOGGER.logInfo("PDUGetStatus", " hMod: %u, hCll: %u, hCoP: %u, StatusCode 0x%x, ret 0x%x", hMod, hCLL, hCoP, *pStatusCode, ret);

	return ret;
}

T_PDU_ERROR __stdcall PDUGetLastError(UNUM32 hMod, UNUM32 hCLL, T_PDU_ERR_EVT* pErrorCode, UNUM32* phCoP, UNUM32* pTimestamp, UNUM32* pExtraErrorInfo)
{
	LOGGER.logWarn("STUB", "PDUGetLastError is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetResourceStatus(PDU_RSC_STATUS_ITEM* pResourceStatus)
{
	LOGGER.logWarn("STUB", "PDUGetResourceStatus is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUCreateComLogicalLink(UNUM32 hMod, PDU_RSC_DATA* pRscData, UNUM32 resourceId, void* pCllTag, UNUM32* phCLL, PDU_FLAG_DATA* pCllCreateFlag)
{
	UNUM32 idx = m_commChannels.size();
	Protocol protocol = CLL_ISO14230;

	auto it = m_objectIdMap.find(pRscData->ProtocolId);
	if (it != m_objectIdMap.end())
	{
		if (it->second == std::string("ISO_14230_3_on_ISO_14230_2"))
		{
			protocol = CLL_ISO14230;
		}
		else if (it->second == std::string("KW82_on_KW_UART"))
		{
			protocol = CLL_KW82;
		}

		LOGGER.logInfo("PDUCreateComLogicalLink", "Detected CLL Protocol %u, DPDU: %s", protocol, it->second.c_str());
	}

	auto cll = std::shared_ptr<ComLogicalLink>(new ComLogicalLink(hMod, idx, m_deviceID, protocol));
	m_commChannels.insert({ idx, cll });

	*phCLL = idx;

	LOGGER.logInfo("PDUCreateComLogicalLink", "hMod %u, BusTypeId %u, ProtocolId %u, NumPinData %u, phCLL %u",
		hMod, pRscData->BusTypeId, pRscData->ProtocolId, pRscData->NumPinData, *phCLL);

	for(int i = 0; i < pRscData->NumPinData; ++i)
	{
		LOGGER.logInfo("PDUCreateComLogicalLink", "    DLCPinNumber %u, DLCPinTypeId 0x%x", pRscData->pDLCPinData[i].DLCPinNumber, pRscData->pDLCPinData[i].DLCPinTypeId);
	}

	if (pCllCreateFlag->NumFlagBytes > 0)
	{
		LOGGER.logInfo("PDUCreateComLogicalLink", "    FlagData 0x%x", pCllCreateFlag->pFlagData[0]);
	}

	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUDestroyComLogicalLink(UNUM32 hMod, UNUM32 hCLL)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;

	LOGGER.logInfo("PDUDestroyComLogicalLink", "hMod %u, hCLL %u", hMod, hCLL);

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		it->second->Disconnect();
		it->second.reset();
		m_commChannels.erase(it);
	}

	return ret;
}

T_PDU_ERROR __stdcall PDUConnect(UNUM32 hMod, UNUM32 hCLL)
{
	T_PDU_ERROR ret = PDU_ERR_CLL_NOT_CONNECTED;
	long cllret = ERR_FAILED;

	LOGGER.logInfo("PDUConnect", "hMod %u, hCLL %u", hMod, hCLL);

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		cllret = it->second->Connect();
	}

	if (cllret == STATUS_NOERROR)
	{
		ret = PDU_STATUS_NOERROR;
	}
	else
	{
		LOGGER.logError("PDUConnect", "hMod %u, hCLL %u, failed: %d", hMod, hCLL, cllret);
	}

	return ret;
}

T_PDU_ERROR __stdcall PDUDisconnect(UNUM32 hMod, UNUM32 hCLL)
{
	T_PDU_ERROR ret = PDU_ERR_CLL_NOT_CONNECTED;
	long cllret = ERR_FAILED;

	LOGGER.logInfo("PDUDisconnect", "hMod %u, hCLL %u", hMod, hCLL);

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		cllret = it->second->Disconnect();
	}

	if (cllret == STATUS_NOERROR)
	{
		ret = PDU_STATUS_NOERROR;
	}
	else
	{
		LOGGER.logError("PDUDisconnect", "hMod %u, hCLL %u, failed: %d", hMod, hCLL, cllret);
	}

	return ret;
}

T_PDU_ERROR __stdcall PDULockResource(UNUM32 hMod, UNUM32 hCLL, UNUM32 LockMask)
{
	LOGGER.logWarn("STUB", "PDULockResource is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUUnlockResource(UNUM32 hMod, UNUM32 hCLL, UNUM32 LockMask)
{
	LOGGER.logWarn("STUB", "PDUUnlockResource is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetComParam(UNUM32 hMod, UNUM32 hCLL, UNUM32 ParamId, PDU_PARAM_ITEM** pParamItem)
{
	LOGGER.logInfo("PDUGetComParam", "hMod %u, hCLL %u, ParamId %u", hMod, hCLL, ParamId);

	if (ParamId == 43)
	{
		*pParamItem = new PDU_PARAM_ITEM;
		(*pParamItem)->ItemType = PDU_IT_PARAM;
		(*pParamItem)->ComParamId = ParamId;
		(*pParamItem)->ComParamClass = PDU_PC_BUSTYPE;
		(*pParamItem)->ComParamDataType = PDU_PT_UNUM32;
		(*pParamItem)->pComParamData = new UNUM32;
		*(UNUM32*)((*pParamItem)->pComParamData) = 4800;
	}

	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUSetComParam(UNUM32 hMod, UNUM32 hCLL, PDU_PARAM_ITEM* pParamItem)
{
	LOGGER.logWarn("PDUSetComParam", "hMod %u, hCLL %u, ComParamId %u, ComParamClass %u, ComParamDataType %u, ComParamData %u",
		hMod, hCLL, pParamItem->ComParamId, pParamItem->ComParamClass, pParamItem->ComParamDataType, *(UNUM32*)(pParamItem->pComParamData));

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		it->second->SetComParam();
	}

	return PDU_STATUS_NOERROR;
}

static UNUM32 m_copCtr = 1;

T_PDU_ERROR __stdcall PDUStartComPrimitive(UNUM32 hMod, UNUM32 hCLL, UNUM32 CoPType, UNUM32 CoPDataSize, UNUM8* pCoPData, PDU_COP_CTRL_DATA* pCopCtrlData, void* pCoPTag, UNUM32* phCoP)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;

	LOGGER.logInfo("PDUStartComPrimitive", "hMod %u, hCLL %u, CoPType 0x%x, CoPDataSize %u, pCoPData %p, pCopCtrlData %p",
		hMod, hCLL, CoPType, CoPDataSize, pCoPData, pCopCtrlData);
	
	if (CoPDataSize > 0 && pCoPData != nullptr)
	{
		std::stringstream ss;
		ss << "CoPData: ";
		for (UNUM32 i = 0; i < CoPDataSize; ++i)
		{
			ss << std::hex << (int)pCoPData[i] << " ";
		}
		LOGGER.logInfo("PDUStartComPrimitive", ss.str().c_str());
	}

	if (pCopCtrlData != nullptr)
	{
		LOGGER.logInfo("PDUStartComPrimitive", "    Time %u, NumSendCycles %d, NumReceiveCycles %d, TempParamUpdate %u, NumPossibleExpectedResponses %u, TxFlag.NumFlagBytes %u",
			pCopCtrlData->Time, pCopCtrlData->NumSendCycles, pCopCtrlData->NumReceiveCycles, pCopCtrlData->TempParamUpdate, pCopCtrlData->NumPossibleExpectedResponses, pCopCtrlData->TxFlag.NumFlagBytes);

		if (pCopCtrlData->NumPossibleExpectedResponses > 0)
		{
			PDU_EXP_RESP_DATA respData = pCopCtrlData->pExpectedResponseArray[0];
			LOGGER.logInfo("PDUStartComPrimitive", "    ResponseType %u, AcceptanceId %u, NumMaskPatternBytes %u, NumUniqueRespIds %u",
				respData.ResponseType, respData.AcceptanceId, respData.NumMaskPatternBytes, respData.NumUniqueRespIds);
		}

		if (pCopCtrlData->TxFlag.NumFlagBytes > 0)
		{
			LOGGER.logInfo("PDUStartComPrimitive", "    pFlagData[0] 0x%x, pFlagData[1] 0x%x, pFlagData[2] 0x%x, pFlagData[3] 0x%x",
				pCopCtrlData->TxFlag.pFlagData[0], pCopCtrlData->TxFlag.pFlagData[1], pCopCtrlData->TxFlag.pFlagData[2], pCopCtrlData->TxFlag.pFlagData[3]);
		}
	}

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		PDU_EVENT_ITEM* pEvt = nullptr;
		switch (CoPType)
		{
		case PDU_COPT_UPDATEPARAM:
			//TODO set comm parameters
			*phCoP = PDU_ID_UNDEF - 1;

			pEvt = new PDU_EVENT_ITEM;
			pEvt->hCop = *phCoP;
			pEvt->ItemType = PDU_IT_STATUS;
			pEvt->pCoPTag = pCoPTag;
			pEvt->pData = new PDU_STATUS_DATA;
			*(PDU_STATUS_DATA*)(pEvt->pData) = PDU_COPST_EXECUTING;
			it->second->SignalEvent(pEvt);

			pEvt = new PDU_EVENT_ITEM;
			pEvt->hCop = *phCoP;
			pEvt->ItemType = PDU_IT_STATUS;
			pEvt->pCoPTag = pCoPTag;
			pEvt->pData = new PDU_STATUS_DATA;
			*(PDU_STATUS_DATA*)(pEvt->pData) = PDU_COPST_FINISHED;
			it->second->SignalEvent(pEvt);

			break;
		case PDU_COPT_STARTCOMM:
		case PDU_COPT_STOPCOMM:
		case PDU_COPT_SENDRECV:
			*phCoP = it->second->StartComPrimitive(CoPType, CoPDataSize, pCoPData, pCopCtrlData, pCoPTag);
			break;
		}
	}
	else
	{
		ret = PDU_ERR_CLL_NOT_CONNECTED;
	}

	LOGGER.logInfo("PDUStartComPrimitive", "Started hCop %u", *phCoP);

	return ret;
}

T_PDU_ERROR __stdcall PDUCancelComPrimitive(UNUM32 hMod, UNUM32 hCLL, UNUM32 hCoP)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;

	LOGGER.logInfo("PDUCancelComPrimitive", "hMod %u, hCLL %u, hCoP %u", hMod, hCLL, hCoP);

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		ret = it->second->Cancel(hCoP);
	}

	return ret;
}

T_PDU_ERROR __stdcall PDUGetEventItem(UNUM32 hMod, UNUM32 hCLL, PDU_EVENT_ITEM** pEventItem)
{
	T_PDU_ERROR ret = PDU_STATUS_NOERROR;

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		bool cllret = it->second->GetEvent(*pEventItem);

		if (!cllret)
		{
			*pEventItem = nullptr;
			ret = PDU_ERR_EVENT_QUEUE_EMPTY;
		}
	}
	else
	{
		*pEventItem = nullptr;
		ret = PDU_ERR_EVENT_QUEUE_EMPTY;
	}

	LOGGER.logWarn("PDUGetEventItem", "hMod %u, hCLL %u, pEvt %p, ret %u", hMod, hCLL, *pEventItem, ret);

	return ret;
}

T_PDU_ERROR __stdcall PDUDestroyItem(PDU_ITEM* pItem)
{
	switch (pItem->ItemType)
	{
		case PDU_IT_IO_UNUM32:
		{
			PDU_DATA_ITEM* pIt = (PDU_DATA_ITEM*)pItem;
			delete pIt->pData;
			pIt->pData = nullptr;
			delete pIt;
			pIt = nullptr;

			LOGGER.logInfo("PDUDestroyItem", "Deleted PDU_IT_IO_UNUM32 pItem %p", pItem);
			break;
		}
		case PDU_IT_STATUS:
		{
			PDU_EVENT_ITEM* pIt = (PDU_EVENT_ITEM*)pItem;
			delete pIt->pData;
			pIt->pData = nullptr;
			delete pIt;
			pIt = nullptr;

			LOGGER.logInfo("PDUDestroyItem", "Deleted PDU_IT_STATUS pItem %p", pItem);
			break;
		}
		case PDU_IT_RESULT:
		{
			PDU_EVENT_ITEM* pIt = (PDU_EVENT_ITEM*)pItem;
			PDU_RESULT_DATA* pData = (PDU_RESULT_DATA*)pIt->pData;
			delete[] pData->pDataBytes;
			pData->pDataBytes = nullptr;
			delete pIt->pData;
			pIt->pData = nullptr;
			delete pIt;
			pIt = nullptr;

			LOGGER.logInfo("PDUDestroyItem", "Deleted PDU_IT_RESULT pItem %p", pItem);
			break;
		}
		case PDU_IT_MODULE_ID:
		{
			PDU_MODULE_ITEM* pIt = (PDU_MODULE_ITEM*)pItem;
			
			//free allocated strings
			PDU_MODULE_DATA** pModules = (PDU_MODULE_DATA**)pIt->pModuleData;
			for (UNUM32 i = 0; i < pIt->NumEntries; ++i)
			{
				delete[] pModules[i]->pVendorModuleName;
				delete[] pModules[i]->pVendorAdditionalInfo;
			}
			
			pIt->NumEntries = 0;
			delete[] pIt->pModuleData;
			pIt->pModuleData = nullptr;
			delete pIt;
			pIt = nullptr;

			LOGGER.logInfo("PDUDestroyItem", "Deleted PDU_IT_MODULE_ID pItem %p", pItem);
			break;
		}
		default:
		{
			LOGGER.logWarn("PDUDestroyItem", "Unhandled pItem %p, ItemType 0x%x", pItem, pItem->ItemType);
		}
	}

	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDURegisterEventCallback(UNUM32 hMod, UNUM32 hCLL, CALLBACKFNC EventCallbackFunction)
{
	LOGGER.logInfo("PDURegisterEventCallback", "hMod %u, hCLL %u", hMod, hCLL);

	auto it = m_commChannels.find(hCLL);
	if (it != m_commChannels.end())
	{
		it->second->RegisterEventCallback(EventCallbackFunction);
	}

	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetObjectId(T_PDU_OBJT pduObjectType, CHAR8* pShortname, UNUM32* pPduObjectId)
{
	UNUM32 id = m_objectIdMap.size();
	m_objectIdMap.insert({ id , std::string(pShortname) });

	*pPduObjectId = id;

	LOGGER.logInfo("PDUGetObjectId", "pduObjectType %d, pShortname %s, pPduObjectId %u", (int)pduObjectType, pShortname, *pPduObjectId);
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetModuleIds(PDU_MODULE_ITEM** pModuleIdList)
{
	// Enumerate J2534 devices
	m_registryList.clear();
	std::set<cPassThruInfo> reg;
	shim_enumPassThruInterfaces(reg);
	std::copy(reg.begin(), reg.end(), std::back_inserter(m_registryList));

	const UNUM32 count = static_cast<UNUM32>(m_registryList.size());

	// Allocate PDU_MODULE_ITEM
	PDU_MODULE_ITEM* moduleItem = new (std::nothrow) PDU_MODULE_ITEM();
	if (!moduleItem)
		return PDU_ERR_FCT_FAILED;

	moduleItem->ItemType = PDU_IT_MODULE_ID;
	moduleItem->NumEntries = 0;
	moduleItem->pModuleData = nullptr;

	if (count == 0)
	{
		*pModuleIdList = moduleItem;
		return PDU_STATUS_NOERROR;
	}

	// Allocate array of PDU_MODULE_DATA
	PDU_MODULE_DATA* modules = new (std::nothrow) PDU_MODULE_DATA[count];
	if (!modules)
		return PDU_ERR_FCT_FAILED;

	UNUM32 idx = 0;
	for (const auto& iface : m_registryList)
	{
		std::string str = iface.Name;

		// Allocate module name
		CHAR8* moduleName = new (std::nothrow) CHAR8[str.size() + 1];
		if (!moduleName)
			return PDU_ERR_FCT_FAILED;

		std::memcpy(moduleName, str.c_str(), str.size() + 1);

		// Allocate additional info
		CHAR8* additionalInfo = new (std::nothrow) CHAR8[32];
		if (!additionalInfo)
			return PDU_ERR_FCT_FAILED;

		std::memset(additionalInfo, 0, 32);
		const std::string infoText = "ConnectionType = 'unknown'";
		std::memcpy(additionalInfo, infoText.c_str(), infoText.size() + 1);

		LOGGER.logInfo("PDUGetModuleIds", "Interface: %s", str.c_str());

		modules[idx].ModuleTypeId = 1;
		modules[idx].hMod = idx;
		modules[idx].pVendorModuleName = moduleName;
		modules[idx].pVendorAdditionalInfo = additionalInfo;
		modules[idx].ModuleStatus = PDU_MODST_AVAIL;

		++idx;
	}

	moduleItem->pModuleData = modules;
	moduleItem->NumEntries = idx;

	*pModuleIdList = moduleItem;

	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetResourceIds(UNUM32 hMod, PDU_RSC_DATA* pResourceIdData, PDU_RSC_ID_ITEM** pResourceIdList)
{
	LOGGER.logWarn("STUB", "PDUGetResourceIds is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetConflictingResources(UNUM32 resourceId, PDU_MODULE_ITEM* pInputModuleList, PDU_RSC_CONFLICT_ITEM** pOutputConflictList)
{
	LOGGER.logWarn("STUB", "PDUGetConflictingResources is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetUniqueRespIdTable(UNUM32 hMod, UNUM32 hCLL, PDU_UNIQUE_RESP_ID_TABLE_ITEM** pUniqueRespIdTable)
{
	LOGGER.logWarn("STUB", "PDUGetUniqueRespIdTable is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUSetUniqueRespIdTable(UNUM32 hMod, UNUM32 hCLL, PDU_UNIQUE_RESP_ID_TABLE_ITEM* pUniqueRespIdTable)
{
	LOGGER.logWarn("STUB", "PDUSetUniqueRespIdTable is unimplimented");
	return PDU_STATUS_NOERROR;
}
