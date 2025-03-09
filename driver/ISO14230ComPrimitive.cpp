#include "pch.h"
#include "pdu_api.h"
#include "ISO14230ComPrimitive.h"
#include "Logger.h"
#include "Settings.h"
#include "j2534/j2534_v0404.h"
#include "j2534/shim_loader.h"

#include <string>
#include <sstream>

constexpr int POLL_TIMEOUT_MS = 10;
constexpr int TIMEOUT_MS = 1000;

const std::vector<UNUM8> MSG_TESTER_PRESENT_41 = { 0x80, 0x41, 0xf1, 0x01, 0x3e, 0xf1 };

long ISO14230ComPrimitive::StartComm(unsigned long channelID, PDU_EVENT_ITEM*& pEvt)
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

	unsigned long dataSize = m_CoPData.size();
	PASSTHRU_MSG txMsg = { m_protocolID, 0, 0, 0, dataSize, dataSize };
	PASSTHRU_MSG rxMsg;

	memcpy(txMsg.Data, &m_CoPData[0], dataSize);

	ret = _PassThruIoctl(channelID, FAST_INIT, &txMsg, &rxMsg);
	if (ret == STATUS_NOERROR)
	{
		--m_CopCtrlData.NumSendCycles;
		--m_CopCtrlData.NumReceiveCycles;

		m_destAddr = m_CoPData[1];

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

long ISO14230ComPrimitive::StopComm(unsigned long channelID, PDU_EVENT_ITEM*& pEvt)
{
	long ret = STATUS_NOERROR;
	return ret;
}

void checksum(std::vector<UNUM8>& data, UNUM32 dataSize)
{
	UNUM8 csum = 0;
	for (UNUM8 i = 0; i < dataSize; ++i)
	{
		csum += data[i];
	}

	data[dataSize] = csum;
}

long ISO14230ComPrimitive::SendRecv(unsigned long channelID, PDU_EVENT_ITEM*& pEvt)
{
	long ret = STATUS_NOERROR;

	if (m_CopCtrlData.NumSendCycles > 0)
	{
		if (TesterPresentWorkaround(pEvt))
		{
			--m_CopCtrlData.NumSendCycles;
			if (m_CopCtrlData.NumReceiveCycles != -1)
			{
				--m_CopCtrlData.NumReceiveCycles;
			}

			return ret;
		}

		if (CheckDestinationAddress(channelID) != STATUS_NOERROR)
		{
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

		unsigned long dataSize = m_CoPData.size();
		PASSTHRU_MSG txMsg = { m_protocolID, 0, 0, 0, dataSize, dataSize };

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
		PASSTHRU_MSG rxMsg = { 0 };
		unsigned long numMsgs = 1;
		ret = _PassThruReadMsgs(channelID, &rxMsg, &numMsgs, POLL_TIMEOUT_MS);
		if (ret == STATUS_NOERROR && rxMsg.RxStatus == START_OF_MESSAGE)
		{
			memset(&rxMsg, 0, sizeof(rxMsg));
			numMsgs = 1;
			ret = _PassThruReadMsgs(channelID, &rxMsg, &numMsgs, TIMEOUT_MS);
			if (ret == STATUS_NOERROR)
			{
				std::stringstream ss;
				ss << "RX: ";
				for (int i = 0; i < rxMsg.DataSize; ++i)
				{
					ss << std::hex << (int)rxMsg.Data[i] << " ";
				}
				ss << "RXStatus: " << std::hex << (int)rxMsg.RxStatus;

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
				pRes->NumDataBytes = rxMsg.DataSize;
				pRes->pDataBytes = new UNUM8[rxMsg.DataSize];
				pRes->pExtraInfo = nullptr;
				pRes->RxFlag.NumFlagBytes = 0;
				pRes->StartMsgTimestamp = 0;
				pRes->TimestampFlags.NumFlagBytes = 0;
				pRes->TxMsgDoneTimestamp = 0;
				pRes->UniqueRespIdentifier = PDU_ID_UNDEF;

				memcpy(pRes->pDataBytes, rxMsg.Data, rxMsg.DataSize);
			}
			else
			{
				LOGGER.logError("ComPrimitive/SendRecv", "_PassThruReadMsgs failed %u", ret);
			}
		}
		else if (ret == ERR_TIMEOUT || ret == ERR_BUFFER_EMPTY)
		{
			LOGGER.logInfo("ComPrimitive/SendRecv", "Timeout while waiting SOM");
			ret = STATUS_NOERROR;
		}
	}

	return ret;
}

long ISO14230ComPrimitive::CheckDestinationAddress(unsigned long channelID)
{
	long ret = STATUS_NOERROR;

	if (Settings::AutoRestartComm)
	{
		if (m_CoPData[1] != m_destAddr)
		{
			LOGGER.logInfo("ComPrimitive/CheckDestinationAddress", "Destination address mismatch, session 0x%x, msg 0x%x", m_destAddr, m_CoPData[1]);
			LOGGER.logInfo("ComPrimitive/CheckDestinationAddress", "Restarting comm to destination 0x%x", m_CoPData[1]);

			std::vector<UNUM8> data = { 0x81, 0x00, 0xf1, 0x81, 0x00 };
			data[1] = m_CoPData[1];
			checksum(data, data.size() - 1);

			PDU_COP_CTRL_DATA ctrlData;
			ctrlData.NumReceiveCycles = 1;
			ctrlData.NumSendCycles = 1;

			auto cop = ISO14230ComPrimitive(PDU_COPT_STARTCOMM, data.size(), data.data(), &ctrlData, nullptr, m_protocolID);

			PDU_EVENT_ITEM* pEvt = nullptr;
			ret = cop.StartComm(channelID, pEvt);
			if (ret == STATUS_NOERROR)
			{
				PDU_EVENT_ITEM* pIt = (PDU_EVENT_ITEM*)pEvt;
				PDU_RESULT_DATA* pData = (PDU_RESULT_DATA*)pIt->pData;
				delete[] pData->pDataBytes;
				pData->pDataBytes = nullptr;
				delete pIt->pData;
				pIt->pData = nullptr;
				delete pIt;
				pIt = nullptr;
			}
		}
	}

	return ret;
}

bool ISO14230ComPrimitive::TesterPresentWorkaround(PDU_EVENT_ITEM*& pEvt)
{
	bool ret = false;

	if (Settings::DisableTesterpresent)
	{
		if (m_CoPData == MSG_TESTER_PRESENT_41)
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

			LOGGER.logInfo("ComPrimitive/TesterPresentWorkaround", "Simulating TesterPresent for DPDU host, message not sent to ECU");

			ret = true;
		}
	}
	else if (Settings::FixTesterpresentDestination)
	{
		if (m_CoPData == MSG_TESTER_PRESENT_41)
		{
			m_CoPData[1] = m_destAddr;
			checksum(m_CoPData, m_CoPData.size() - 1);

			LOGGER.logInfo("ComPrimitive/TesterPresentWorkaround", "TesterPresent destination fixed to 0x%x", m_destAddr);

			ret = false;
		}
	}

	return ret;
}