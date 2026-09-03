#include "pch.h"
#include "pdu_api.h"
#include "ComPrimitive.h"
#include "Logger.h"
#include "Settings.h"
#include "j2534/j2534_v0404.h"
#include "j2534/shim_loader.h"

#include <string>
#include <sstream>

UNUM32 ComPrimitive::m_hCoPCtr = 0;
UNUM8 ComPrimitive::m_destAddr = 0;

ComPrimitive::ComPrimitive(UNUM32 CoPType, UNUM32 CoPDataSize, UNUM8* pCoPData, PDU_COP_CTRL_DATA* pCopCtrlData,
                           void* pCoPTag, unsigned long protocolID) :
	m_state(PDU_COPST_IDLE), m_CoPType(CoPType), m_pCoPTag(pCoPTag), m_protocolID(protocolID)
{
	if (pCoPData != nullptr && CoPDataSize > 0)
	{
		m_CoPData = std::vector<UNUM8>(pCoPData, pCoPData + CoPDataSize);
	}

	if (pCopCtrlData != nullptr)
	{
		m_CopCtrlData = *pCopCtrlData;
	}
	else
	{
		memset(&m_CopCtrlData, 0, sizeof(m_CopCtrlData));
	}

	if (m_hCoPCtr == 0) //hcop 0 is invalid
	{
		++m_hCoPCtr;
	}
	m_hCoP = m_hCoPCtr++;

}

UNUM32 ComPrimitive::getHandle()
{
	const std::lock_guard<std::mutex> lock(m_stateLock);
	return m_hCoP;
}

UNUM32 ComPrimitive::getType()
{
	return m_CoPType;
}

void ComPrimitive::Execute(PDU_EVENT_ITEM*& pEvt)
{
	const std::lock_guard<std::mutex> lock(m_stateLock);
	if (m_state != PDU_COPST_EXECUTING)
	{
		m_state = PDU_COPST_EXECUTING;
		GenerateStatusEvent(pEvt);
	}
}

T_PDU_STATUS ComPrimitive::GetStatus()
{
	const std::lock_guard<std::mutex> lock(m_stateLock);
	return m_state;
}

void ComPrimitive::Cancel(PDU_EVENT_ITEM*& pEvt)
{
	const std::lock_guard<std::mutex> lock(m_stateLock);
	if (m_state != PDU_COPST_CANCELLED)
	{
		m_state = PDU_COPST_CANCELLED;
		GenerateStatusEvent(pEvt);
	}
}

void ComPrimitive::Destroy()
{
	const std::lock_guard<std::mutex> lock(m_stateLock);
	LOGGER.logInfo("ComPrimitive/Destroy", "hCoP %u", m_hCoP);
	m_hCoP = 0;
}

void ComPrimitive::Finish(PDU_EVENT_ITEM*& pEvt)
{
	const std::lock_guard<std::mutex> lock(m_stateLock);
	if (m_CopCtrlData.NumSendCycles == 0 && m_CopCtrlData.NumReceiveCycles == 0)
	{
		if (m_state != PDU_COPST_FINISHED)
		{
			m_state = PDU_COPST_FINISHED;
			GenerateStatusEvent(pEvt);
		}
	}
}

void ComPrimitive::GenerateStatusEvent(PDU_EVENT_ITEM*& pEvt)
{
	pEvt = new PDU_EVENT_ITEM;
	pEvt->hCop = m_hCoP;
	pEvt->ItemType = PDU_IT_STATUS;
	pEvt->pCoPTag = m_pCoPTag;
	pEvt->pData = new PDU_STATUS_DATA;
	*(PDU_STATUS_DATA*)(pEvt->pData) = m_state;
}
