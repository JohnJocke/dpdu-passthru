#include "pch.h"
#include "pdu_api.h"
#include "ComPrimitive.h"
#include "Logger.h"
#include "j2534/j2534_v0404.h"
#include "j2534/shim_loader.h"

#include <string>
#include <sstream>

constexpr int TIMEOUT_MS = 500;

UNUM32 ComPrimitive::m_hCoPCtr = 0;

ComPrimitive::ComPrimitive(UNUM32 CoPType, UNUM32 CoPDataSize, UNUM8* pCoPData, PDU_COP_CTRL_DATA* pCopCtrlData, void* pCoPTag) :
	m_state(PDU_COPST_IDLE), m_CoPType(CoPType), m_pCoPTag(pCoPTag)
{
	m_CoPData = std::vector<UNUM8>(pCoPData, pCoPData + CoPDataSize);
	m_CopCtrlData = *pCopCtrlData;

	if (m_hCoPCtr == 0) //hcop 0 is invalid
	{
		++m_hCoPCtr;
	}
	m_hCoP = m_hCoPCtr++;

}

UNUM32 ComPrimitive::getHandle()
{
	return m_hCoP;
}

UNUM32 ComPrimitive::getType()
{
	return m_CoPType;
}

long ComPrimitive::StartComm(unsigned long channelID, PDU_EVENT_ITEM* & pEvt)
{
	long ret = STATUS_NOERROR;

	if (m_CopCtrlData.NumReceiveCycles == 0 || m_CopCtrlData.NumSendCycles == 0)
	{
		LOGGER.logInfo("ComPrimitive/StartComm", "finished NumReceiveCycles %u, NumSendCycles %u",
			m_CopCtrlData.NumReceiveCycles, m_CopCtrlData.NumSendCycles);
		return ret;
	}

	std::stringstream ss;
	ss << "TX: ";
	for (int i = 0; i < m_CoPData.size(); ++i)
	{
		ss << std::hex << (int)m_CoPData[i] << " ";
	}
	LOGGER.logInfo("ComPrimitive/StartComm", ss.str().c_str());

	//NOTE: Checksum skipped
	unsigned long dataSize = m_CoPData.size() - 1;

	PASSTHRU_MSG txMsg = { channelID, 0, 0, 0, dataSize, dataSize };
	PASSTHRU_MSG rxMsg;

	memcpy(txMsg.Data, &m_CoPData[0], dataSize);

	ret = _PassThruIoctl(channelID, FAST_INIT, &txMsg, &rxMsg);
	if (ret == STATUS_NOERROR)
	{
		--m_CopCtrlData.NumSendCycles;
		--m_CopCtrlData.NumReceiveCycles;

		std::stringstream ss;
		ss << "RX: ";
		for (int i = 0; i < rxMsg.DataSize; ++i)
		{
			ss << std::hex << (int)rxMsg.Data[i] << " ";
		}
		LOGGER.logInfo("ComPrimitive/StartComm", ss.str().c_str());

		pEvt = new PDU_EVENT_ITEM;
		pEvt->hCop = m_hCoP;
		pEvt->ItemType = PDU_IT_RESULT;
		pEvt->pCoPTag = m_pCoPTag;
		pEvt->pData = new PDU_RESULT_DATA;

		PDU_RESULT_DATA* pRes = (PDU_RESULT_DATA*)(pEvt->pData);
		pRes->AcceptanceId = 1;
		pRes->NumDataBytes = rxMsg.DataSize;
		pRes->pDataBytes = new UNUM8[rxMsg.DataSize];
		pRes->pExtraInfo = nullptr;
		pRes->RxFlag.NumFlagBytes = 0;
		pRes->StartMsgTimestamp = 0;
		pRes->TimestampFlags.NumFlagBytes = 0;
		pRes->TxMsgDoneTimestamp = 0;
		pRes->UniqueRespIdentifier = PDU_ID_UNDEF;

		memcpy(pRes->pDataBytes, rxMsg.Data, pRes->NumDataBytes);
	}

	return ret;
}

void checksum(UNUM8* data, UNUM32 dataSize)
{
	UNUM8 csum = 0;
	for (UNUM8 i = 0; i < dataSize; ++i)
	{
		csum += data[i];
	}

	data[dataSize] = csum;
}

long ComPrimitive::SendRecv(unsigned long channelID, PDU_EVENT_ITEM*& pEvt)
{
	long ret = STATUS_NOERROR;

	if (m_CopCtrlData.NumSendCycles > 0)
	{
		if (TesterPresentSimulation(pEvt))
		{
			--m_CopCtrlData.NumSendCycles;
			if (m_CopCtrlData.NumReceiveCycles != -1)
			{
				--m_CopCtrlData.NumReceiveCycles;
			}

			return ret;
		}

		std::stringstream ss;
		ss << "TX: ";
		for (int i = 0; i < m_CoPData.size(); ++i)
		{
			ss << std::hex << (int)m_CoPData[i] << " ";
		}
		LOGGER.logInfo("ComPrimitive/SendRecv", ss.str().c_str());

		unsigned long numMsgs = 1;

		//NOTE: Checksum skipped
		unsigned long dataSize = m_CoPData.size() - 1;
		PASSTHRU_MSG txMsg = { channelID, 0, 0, 0, dataSize, dataSize };

		memcpy(txMsg.Data, &m_CoPData[0], dataSize);

		ret = _PassThruWriteMsgs(channelID, &txMsg, &numMsgs, TIMEOUT_MS);

		if (ret == STATUS_NOERROR)
		{
			--m_CopCtrlData.NumSendCycles;
		}
		else
		{
			LOGGER.logError("ComPrimitive/SendRecv", "_PassThruWriteMsgs failed %u", ret);
		}

	}

	if (m_CopCtrlData.NumReceiveCycles > 0 || m_CopCtrlData.NumReceiveCycles == -1)
	{
		PASSTHRU_MSG rxMsg[2];
		unsigned long numMsgs = 2;
		ret = _PassThruReadMsgs(channelID, rxMsg, &numMsgs, TIMEOUT_MS);
		if (ret == STATUS_NOERROR)
		{
			std::stringstream ss;
			ss << "RX: ";
			for (int i = 0; i < rxMsg[1].DataSize; ++i)
			{
				ss << std::hex << (int)rxMsg[1].Data[i] << " ";
			}
			ss << "RXStatus: " << std::hex << (int)rxMsg[1].RxStatus;

			LOGGER.logInfo("ComPrimitive/SendRecv", ss.str().c_str());

			if (m_CopCtrlData.NumReceiveCycles != -1)
			{
				--m_CopCtrlData.NumReceiveCycles;
			}

			pEvt = new PDU_EVENT_ITEM;
			pEvt->hCop = m_hCoP;
			pEvt->ItemType = PDU_IT_RESULT;
			pEvt->pCoPTag = m_pCoPTag;
			pEvt->pData = new PDU_RESULT_DATA;

			PDU_RESULT_DATA* pRes = (PDU_RESULT_DATA*)(pEvt->pData);
			pRes->AcceptanceId = 1;
			pRes->NumDataBytes = rxMsg[1].DataSize + 1;
			pRes->pDataBytes = new UNUM8[rxMsg[1].DataSize + 1];
			pRes->pExtraInfo = nullptr;
			pRes->RxFlag.NumFlagBytes = 0;
			pRes->StartMsgTimestamp = 0;
			pRes->TimestampFlags.NumFlagBytes = 0;
			pRes->TxMsgDoneTimestamp = 0;
			pRes->UniqueRespIdentifier = PDU_ID_UNDEF;

			memcpy(pRes->pDataBytes, rxMsg[1].Data, rxMsg[1].DataSize);

			checksum(pRes->pDataBytes, rxMsg[1].DataSize);

			ss.str("");
			ss << "RX csum: ";
			for (int i = 0; i < pRes->NumDataBytes; ++i)
			{
				ss << std::hex << (int)pRes->pDataBytes[i] << " ";
			}
			LOGGER.logInfo("ComPrimitive/SendRecv", ss.str().c_str());
		}
		else
		{
			LOGGER.logError("ComPrimitive/SendRecv", "_PassThruReadMsgs failed %u", ret);
		}
	}

	return ret;
}

void ComPrimitive::Execute(PDU_EVENT_ITEM*& pEvt)
{
	if (m_state != PDU_COPST_EXECUTING)
	{
		m_state = PDU_COPST_EXECUTING;
		GenerateStatusEvent(pEvt);
	}
}

T_PDU_STATUS ComPrimitive::GetStatus()
{
	return m_state;
}

void ComPrimitive::Cancel(PDU_EVENT_ITEM*& pEvt)
{
	if (m_state != PDU_COPST_CANCELLED)
	{
		m_state = PDU_COPST_CANCELLED;
		GenerateStatusEvent(pEvt);
	}
}

void ComPrimitive::Destroy()
{
	LOGGER.logInfo("ComPrimitive/Destroy", "hCoP %u", m_hCoP);
	m_hCoP = 0;
}

void ComPrimitive::Finish(PDU_EVENT_ITEM*& pEvt)
{
	if (m_CopCtrlData.NumSendCycles == 0 && m_CopCtrlData.NumReceiveCycles == 0)
	{
		if (m_state != PDU_COPST_FINISHED)
		{
			m_state = PDU_COPST_FINISHED;
			GenerateStatusEvent(pEvt);
		}
	}
}

bool ComPrimitive::TesterPresentSimulation(PDU_EVENT_ITEM*& pEvt)
{
	bool ret = false;

	if (m_CoPData.size() >= 6 && m_CoPData[4] == 0x3e)
	{
		pEvt = new PDU_EVENT_ITEM;
		pEvt->hCop = m_hCoP;
		pEvt->ItemType = PDU_IT_RESULT;
		pEvt->pCoPTag = m_pCoPTag;
		pEvt->pData = new PDU_RESULT_DATA;

		PDU_RESULT_DATA* pRes = (PDU_RESULT_DATA*)(pEvt->pData);
		pRes->AcceptanceId = 1;
		pRes->NumDataBytes = 5;
		pRes->pDataBytes = new UNUM8[5]{ 0x81, 0xf1, 0x41, 0x7e, 0x31 };
		pRes->pExtraInfo = nullptr;
		pRes->RxFlag.NumFlagBytes = 0;
		pRes->StartMsgTimestamp = 0;
		pRes->TimestampFlags.NumFlagBytes = 0;
		pRes->TxMsgDoneTimestamp = 0;
		pRes->UniqueRespIdentifier = PDU_ID_UNDEF;

		LOGGER.logInfo("ComPrimitive/TesterPresentSimulation", "Simulating TesterPresent");

		ret = true;
	}

	return ret;
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
