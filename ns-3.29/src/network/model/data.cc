#include "data.h"

namespace ns3 {

Data::Data() {
	m_genTime = 0;
	m_arrTime = 0;
	m_priority = 0;
	m_dataType = 0;
	m_nodeId = 0;
	m_data = 0;
}

Data::~Data() {
	
}

uint64_t Data::GetGenTime (void) const {
	return m_genTime;
}
void Data::SetGenTime (uint64_t genTime) {
	m_genTime = genTime;
}

uint64_t Data::GetArrTime (void) const {
	return m_arrTime;
}
void Data::SetArrTime (uint64_t arrTime) {
	m_arrTime = arrTime;
}

uint8_t Data::GetPriority (void) const {
	return m_priority;
}
void Data::SetPriority (uint8_t priority) {
	m_priority = priority;
}

uint8_t Data::GetDataType (void) const {
	return m_dataType;
}
void Data::SetDataType (uint8_t dataType) {
	m_dataType = dataType;
}

uint16_t Data::GetNodeId (void) const {
	return m_nodeId;
}
void Data::SetNodeId (uint16_t nodeId) {
	m_nodeId = nodeId;
}

uint16_t Data::GetData (void) const {
	return m_data;
}
void Data::SetData (uint16_t data) {
	m_data = data;
}
}