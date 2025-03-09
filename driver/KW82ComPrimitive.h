#pragma once
#include "ComPrimitive.h"
class KW82ComPrimitive : public ComPrimitive
{
public:
	KW82ComPrimitive(UNUM32 CoPType, UNUM32 CoPDataSize, UNUM8* pCoPData, PDU_COP_CTRL_DATA* pCopCtrlData,
		void* pCoPTag, unsigned long protocolID) :
		ComPrimitive(CoPType, CoPDataSize, pCoPData, pCopCtrlData, pCoPTag, protocolID)
	{
	}

	virtual long StartComm(unsigned long channelID, PDU_EVENT_ITEM*& pEvt);
	virtual long StopComm(unsigned long channelID, PDU_EVENT_ITEM*& pEvt);
	virtual long SendRecv(unsigned long channelID, PDU_EVENT_ITEM*& pEvt);
};

