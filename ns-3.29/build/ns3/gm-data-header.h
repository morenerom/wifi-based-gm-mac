#ifndef GM_DATA_HEADER_H
#define GM_DATA_HEADER_H

#include "ns3/header.h"
#include <vector>

using namespace std;

#define DATA_SIZE 28

namespace ns3 {

enum DataType {
	TEMPERATURE = 0,
	HUMIDITY,
};

class GmDataHeader : public Header 
 {
 public:
 
   GmDataHeader ();
   virtual ~GmDataHeader ();
 
   void AddGenTime (int64_t sensedGenTime);
   vector<int64_t> GetSensedGenTime (void) const;
   void AddPriority (uint8_t sensedPriority);
   vector<uint8_t> GetSensedPriority (void) const;
   void AddDataType (uint8_t sensedDataType);
   vector<uint8_t> GetSensedDataType (void) const;
   void AddNodeId (uint16_t sendsedNodeId);
   vector<uint16_t> GetSensedNodeId (void) const;
   void AddData (uint16_t sensedData);
   vector<uint16_t> GetSensedData (void) const;
   void AddX (double x);
   vector<double> GetX (void) const;
   void AddY (double y);
   vector<double> GetY (void) const;
 
   static TypeId GetTypeId (void);
   virtual TypeId GetInstanceTypeId (void) const;
   virtual void Print (std::ostream &os) const;
   virtual void Serialize (Buffer::Iterator start) const;
   virtual uint32_t Deserialize (Buffer::Iterator start);
   virtual uint32_t GetSerializedSize (void) const;
 private:
   vector<int64_t> m_sensedGenTime;			// the time when data is collected
   vector<uint8_t> m_sensedPriority;		// for mananging urgency
   vector<uint8_t> m_sensedDataType;		// the type of data (temperature, humidity)
   vector<uint16_t> m_sensedNodeId;			// the own number of nodes
   vector<uint16_t> m_sensedData;
   vector<double> m_x;
   vector<double> m_y;
 };
}

#endif