#ifndef DATA_H
#define DATA_H

#include "ns3/nstime.h"
#include "ns3/gm-data-header.h"

namespace ns3 {

class Data
{
public:
	Data();
	~Data();

	uint64_t GetGenTime (void) const;
	void SetGenTime (uint64_t genTime);
	uint64_t GetArrTime (void) const;
	void SetArrTime (uint64_t arrTime);
	uint8_t GetPriority (void) const;
	void SetPriority (uint8_t priority);
	uint8_t GetDataType (void) const;
	void SetDataType (uint8_t dataType);
	uint16_t GetNodeId (void) const;
	void SetNodeId (uint16_t nodeId);
	uint16_t GetData (void) const;
	void SetData (uint16_t data);
private:
	uint64_t m_genTime;
	uint64_t m_arrTime;
	uint8_t m_priority;
	uint8_t m_dataType;
	uint16_t m_nodeId;
	uint16_t m_data;
};
}
#endif