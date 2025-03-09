#pragma once

#include "pdu_api.h"
#include <vector>

class ComPrimitive
{
public:
	ComPrimitive(UNUM32 CoPType, UNUM32 CoPDataSize, UNUM8* pCoPData, PDU_COP_CTRL_DATA* pCopCtrlData, void* pCoPTag, unsigned long m_protocolID);

	UNUM32 getHandle();
	UNUM32 getType();

	virtual long StartComm(unsigned long channelID, PDU_EVENT_ITEM* & pEvt) = 0;
	virtual long StopComm(unsigned long channelID, PDU_EVENT_ITEM*& pEvt) = 0;
	virtual long SendRecv(unsigned long channelID, PDU_EVENT_ITEM*& pEvt) = 0;

	T_PDU_STATUS GetStatus();

	void Execute(PDU_EVENT_ITEM*& pEvt);
	void Finish(PDU_EVENT_ITEM*& pEvt);
	void Cancel(PDU_EVENT_ITEM*& pEvt);
	void Destroy();

protected:
	void GenerateStatusEvent(PDU_EVENT_ITEM*& pEvt);

	T_PDU_STATUS m_state;

	UNUM32 m_hCoP;
	UNUM32 m_CoPType;
	std::vector<UNUM8> m_CoPData;
	PDU_COP_CTRL_DATA m_CopCtrlData;
	void* m_pCoPTag;

	unsigned long m_protocolID;

	static UNUM32 m_hCoPCtr;
	static UNUM8 m_destAddr;

};

