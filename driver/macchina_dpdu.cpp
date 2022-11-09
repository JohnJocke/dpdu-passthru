#include "pch.h"
#include "macchina_dpdu.h"
#include "Logger.h"
#include "pdu_api.h"
#include <chrono>


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
	LOGGER.logWarn("STUB", "PDUModuleConnect is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUModuleDisconnect(UNUM32 hMod)
{
	LOGGER.logWarn("STUB", "PDUModuleDisconnect is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetTimestamp(UNUM32 hMod, UNUM32* pTimestamp)
{
	LOGGER.logWarn("STUB", "PDUGetTimestamp is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUIoCtl(UNUM32 hMod, UNUM32 hCLL, UNUM32 IoCtlCommandId, PDU_DATA_ITEM* pInputData, PDU_DATA_ITEM** pOutputData)
{
	LOGGER.logWarn("STUB", "PDUIoCtl is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetVersion(UNUM32 hMod, PDU_VERSION_DATA* pVersionData)
{
	LOGGER.logWarn("STUB", "PDUGetVersion is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetStatus(UNUM32 hMod, UNUM32 hCLL, UNUM32 hCoP, T_PDU_STATUS* pStatusCode, UNUM32* pTimestamp, UNUM32* pExtraInfo)
{
	LOGGER.logWarn("STUB", "PDUGetStatus is unimplimented. hMod: %lu, hCll: %lu, hCoP: %lu", hMod, hCLL, hCoP);
	*pStatusCode = T_PDU_STATUS::PDU_MODST_READY;
	if (pExtraInfo != nullptr) {
		*pExtraInfo = 0;
	}
	return PDU_STATUS_NOERROR;
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

T_PDU_ERROR __stdcall PDUCreateComLogicalLink(UNUM32 hMod, PDU_RSC_DATA* pRscData, UNUM32 resourceId, void* pCllTag, UNUM32* phCLL, PDU_FLAG_DATA* CllCreateFlag)
{
	LOGGER.logWarn("STUB", "PDUCreateComLogicalLink is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUDestroyComLogicalLink(UNUM32 hMod, UNUM32 hCLL)
{
	LOGGER.logWarn("STUB", "PDUDestroyComLogicalLink is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUConnect(UNUM32 hMod, UNUM32 hCLL)
{
	LOGGER.logWarn("STUB", "PDUConnect is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUDisconnect(UNUM32 hMod, UNUM32 hCLL)
{
	LOGGER.logWarn("STUB", "PDUDisconnect is unimplimented");
	return PDU_STATUS_NOERROR;
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
	LOGGER.logWarn("STUB", "PDUGetComParam is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUSetComParam(UNUM32 hMod, UNUM32 hCLL, PDU_PARAM_ITEM* pParamItem)
{
	LOGGER.logWarn("STUB", "PDUSetComParam is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUStartComPrimitive(UNUM32 hMod, UNUM32 hCLL, UNUM32 CoPType, UNUM32 CoPDataSize, UNUM8* pCoPData, PDU_COP_CTRL_DATA* pCopCtrlData, void* pCoPTag, UNUM32* phCoP)
{
	LOGGER.logWarn("STUB", "PDUStartComPrimitive is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUCancelComPrimitive(UNUM32 hMod, UNUM32 hCLL, UNUM32 hCoP)
{
	LOGGER.logWarn("STUB", "PDUCancelComPrimitive is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetEventItem(UNUM32 hMod, UNUM32 hCLL, PDU_EVENT_ITEM** pEventItem)
{
	LOGGER.logWarn("STUB", "PDUGetEventItem is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUDestroyItem(PDU_ITEM* pItem)
{
	LOGGER.logWarn("STUB", "PDUDestroyItem is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDURegisterEventCallback(UNUM32 hMod, UNUM32 hCLL, CALLBACKFNC EventCallbackFunction)
{
	LOGGER.logWarn("STUB", "PDURegisterEventCallback is unimplimented");
	return PDU_STATUS_NOERROR;
}

T_PDU_ERROR __stdcall PDUGetObjectId(T_PDU_OBJT pduObjectType, CHAR8* pShortname, UNUM32* pPduObjectId)
{
	LOGGER.logWarn("STUB", "PDUGetObjectId is unimplimented");
	return PDU_STATUS_NOERROR;
}

// -- Testing --
PDU_MODULE_DATA d = {
	1,
	1,
	(CHAR8*)"Macchina M2 UTD",
	(CHAR8*)"github.com/rnd-ash",
	PDU_MODST_AVAIL
};

PDU_MODULE_ITEM x = {
	PDU_IT_MODULE_ID,
	1,
	&d
};

T_PDU_ERROR __stdcall PDUGetModuleIds(PDU_MODULE_ITEM** pModuleIdList)
{
	//pModuleIdList = new PDU_MODULE_ITEM*[1];
	*pModuleIdList = &x;
	LOGGER.logWarn("STUB", "PDUGetModuleIds is unimplimented");
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
