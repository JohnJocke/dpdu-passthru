#include "pch.h"
#include "pdu_api.h"
#include "KW82ComPrimitive.h"
#include "Logger.h"
#include "Settings.h"
#include "j2534/j2534_v0404.h"
#include "j2534/shim_loader.h"

#include <string>
#include <sstream>

constexpr int POLL_TIMEOUT_MS = 10;
constexpr int TIMEOUT_MS = 1000;

long KW82ComPrimitive::StartComm(unsigned long channelID, PDU_EVENT_ITEM*& pEvt)
{
	long ret = STATUS_NOERROR;

	if (m_CopCtrlData.NumReceiveCycles == 0 || m_CopCtrlData.NumSendCycles == 0)
	{
		LOGGER.logInfo("ComPrimitive/StartComm", "finished NumReceiveCycles %u, NumSendCycles %u",
			m_CopCtrlData.NumReceiveCycles, m_CopCtrlData.NumSendCycles);
		return ret;
	}

	LOGGER.logInfo("ComPrimitive/StartComm", "Starting five baud init");

	_SBYTE_ARRAY input;
	_SBYTE_ARRAY output;
	UINT8 ecuAddress = 0x64;
	UINT8 keyword[2] = { 0, 0 };

	input.NumOfBytes = 1;
	input.BytePtr = &ecuAddress;

	output.NumOfBytes = 2;
	output.BytePtr = keyword;

	ret = _PassThruIoctl(channelID, FIVE_BAUD_INIT, &input, &output);
	if (ret == STATUS_NOERROR)
	{
		LOGGER.logInfo("ComPrimitive/StartComm", "Connected to ECU, keywords: %x %x", keyword[0], keyword[1]);

		PASSTHRU_MSG rxMsg = { 0 };
		unsigned long numMsgs = 1;
		ret = _PassThruReadMsgs(channelID, &rxMsg, &numMsgs, TIMEOUT_MS);
		if (ret == STATUS_NOERROR && rxMsg.RxStatus == START_OF_MESSAGE)
		{
			memset(&rxMsg, 0, sizeof(rxMsg));
			numMsgs = 1;
			ret = _PassThruReadMsgs(channelID, &rxMsg, &numMsgs, TIMEOUT_MS);
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
		}
		else
		{
			LOGGER.logError("ComPrimitive/StartComm", "_PassThruReadMsgs failed %u", ret);
		}
		
	}

	return ret;
}

long KW82ComPrimitive::StopComm(unsigned long channelID, PDU_EVENT_ITEM*& pEvt)
{
	long ret = STATUS_NOERROR;

	LOGGER.logInfo("ComPrimitive/StopComm", "Terminating session");
	unsigned long dataSize = 4;
	PASSTHRU_MSG txMsg = { m_protocolID, 0, 0, 0, dataSize, dataSize };
	txMsg.Data[0] = 0x02;
	txMsg.Data[1] = 0xB2;
	txMsg.Data[2] = 0x00;
	txMsg.Data[3] = 0xB4;

	//spam "end session" a few times
	for (int i = 0; i < 5; ++i)
	{
		unsigned long numMsgs = 1;
		ret = _PassThruWriteMsgs(channelID, &txMsg, &numMsgs, TIMEOUT_MS);
		if (ret != STATUS_NOERROR)
		{
			LOGGER.logError("ComPrimitive/StartComm", "_PassThruWriteMsgs failed %u", ret);
		}
	}

	return ret;
}

long KW82ComPrimitive::SendRecv(unsigned long channelID, PDU_EVENT_ITEM*& pEvt)
{
	long ret = STATUS_NOERROR;

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

	if (m_CopCtrlData.NumSendCycles > 0)
	{
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

	return STATUS_NOERROR;
}
