#include "gm-data-header.h"
#include "ns3/log-macros-enabled.h"

namespace ns3 {
GmDataHeader::GmDataHeader ()
 {
  m_sensedGenTime.clear();      // the time when data is collected
  m_sensedPriority.clear();    // for mananging urgency
  m_sensedDataType.clear();    // the type of data (temperature, humidity)
  m_sensedNodeId.clear();     // the own number of nodes
  m_sensedData.clear();
 }
 GmDataHeader::~GmDataHeader ()
 {
 }

void GmDataHeader::AddGenTime (int64_t genTime) {
	m_sensedGenTime.push_back(genTime);
}
vector<int64_t> GmDataHeader::GetSensedGenTime (void) const {
	return m_sensedGenTime;
}

void GmDataHeader::AddPriority (uint8_t priority) {
	m_sensedPriority.push_back(priority);
}
vector<uint8_t> GmDataHeader::GetSensedPriority (void) const {
	return m_sensedPriority;
}

void GmDataHeader::AddDataType (uint8_t dataType) {
  m_sensedDataType.push_back(dataType);
}
vector<uint8_t> GmDataHeader::GetSensedDataType (void) const {
  return m_sensedDataType;
}

void GmDataHeader::AddNodeId (uint16_t nodeId) {
  m_sensedNodeId.push_back(nodeId);
}
vector<uint16_t> GmDataHeader::GetSensedNodeId (void) const {
	return m_sensedNodeId;
}

void GmDataHeader::AddData (uint16_t data) {
  m_sensedData.push_back(data);
}
vector<uint16_t> GmDataHeader::GetSensedData (void) const {
  return m_sensedData;
}

 TypeId
 GmDataHeader::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::GmDataHeader")
     .SetParent<Header> ()
     .AddConstructor<GmDataHeader> ()
   ;
   return tid;
 }
 TypeId
 GmDataHeader::GetInstanceTypeId (void) const
 {
   return GetTypeId ();
 }
 
 void
 GmDataHeader::Print (std::ostream &os) const
 {
   
 }
 uint32_t
 GmDataHeader::GetSerializedSize (void) const
 {
   return m_sensedData.size() * 12;   // 12 is the size of sensed data
 }
 void
 GmDataHeader::Serialize (Buffer::Iterator start) const
 {
   // we can serialize two bytes at the start of the buffer.
   // we write them in network byte order.
  uint16_t val;
  for(uint16_t i=0;i<m_sensedData.size();i++) {
    val = 0;
    val |= (m_sensedPriority[i]) & (0x03);
    val |= (m_sensedDataType[i] << 2) & (0x07 << 2);//GM-MAC : shifting&addition
    val |= (m_sensedNodeId[i] << 5) & (0x7ff << 5);
    start.WriteHtolsbU16 (val);
    start.WriteHtolsbU64 (m_sensedGenTime[i]);
    start.WriteHtolsbU16 (m_sensedData[i]);
  }
 }
 uint32_t
 GmDataHeader::Deserialize (Buffer::Iterator start)
 {
   // we can deserialize two bytes from the start of the buffer.
   // we read them in network byte order and store them
   // in host byte order.
    Buffer::Iterator s = start;
    int len = s.GetSize() / 12;//GM-MAC :12 is 12bytes. if there is another data, 12 will be edited
    //NS_LOG_UNCOND("len : " << len);
    uint16_t val;
    for(int i = 0; i < len; i++) {
      val = s.ReadLsbtohU16();
      m_sensedPriority.push_back((val) & (0x03));
      m_sensedDataType.push_back((val >> 2) & (0x07));
      m_sensedNodeId.push_back((val >> 5) & (0x7ff));
      m_sensedGenTime.push_back(s.ReadLsbtohU64());
      m_sensedData.push_back(s.ReadLsbtohU16());
    }
 
   // we return the number of bytes effectively read.
    return s.GetDistanceFrom (start);
 }
}