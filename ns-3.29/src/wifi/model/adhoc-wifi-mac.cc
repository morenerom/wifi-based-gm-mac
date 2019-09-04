/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/packet.h"
#include "adhoc-wifi-mac.h"
#include "ht-capabilities.h"
#include "vht-capabilities.h"
#include "he-capabilities.h"
#include "mac-low.h"
#include "ns3/ipv4-header.h"

#include "ns3/simulator.h"
#include "ns3/gm-data-header.h"
#include "ns3/data.h"

#define periodicResettingThreshold 1000

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AdhocWifiMac");

NS_OBJECT_ENSURE_REGISTERED (AdhocWifiMac);

TypeId
AdhocWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AdhocWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<AdhocWifiMac> ()
  ;
  return tid;
}

AdhocWifiMac::AdhocWifiMac ()
{
  NS_LOG_FUNCTION (this);
  //Let the lower layers know that we are acting in an IBSS
  SetTypeOfStation (ADHOC_STA);
  m_stateType = INIT;
  m_requestFailCount = 0;
  m_activeNum = 0;

  m_low->SetStateCallback (MakeCallback (&AdhocWifiMac::SetStateType, this));
  m_low->SetEnqueueCallback (MakeCallback (&AdhocWifiMac::Enqueue, this));
  m_low->CancelTACallback (MakeCallback(&AdhocWifiMac::CancelTA, this));
}

AdhocWifiMac::~AdhocWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void AdhocWifiMac::StartRequest(void) {
  SetStateType(REQUEST);
  Ptr<Packet> packet = Create<Packet> (0);
  Mac48Address m("ff:ff:ff:ff:ff:ff");
  Enqueue(packet, m);
}

void AdhocWifiMac::CancelTA (void) {
  //NS_LOG_UNCOND(Simulator::Now() << " AdhocWifiMac::CancelTA");
  m_TAId.Cancel();
}

StateType AdhocWifiMac::GetStateType (void) {
  return m_stateType;
}
void AdhocWifiMac::SetStateType (StateType stateType) {
  m_stateType = stateType;
  if(stateType == REQUEST)
    m_receivedInfo.clear();
}

void
AdhocWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  //In an IBSS, the BSSID is supposed to be generated per Section
  //11.1.3 of IEEE 802.11. We don't currently do this - instead we
  //make an IBSS STA a bit like an AP, with the BSSID for frames
  //transmitted by each STA set to that STA's address.
  //
  //This is why we're overriding this method.
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}
// GM-MAC
void AdhocWifiMac::DataTransmission(void) {
  Ptr<Node> node = m_stationManager->GetNode();

  cout << "DataTransmission " << Simulator::Now() << " " << m_low->GetAddress() << " -> " << node->GetParentMacAddress() << " : " << node->GetDataAmount() << '\n'; 
  GmDataHeader h;
  Ptr<Packet> p = Create<Packet> (0);
  vector<Data*> curNodeBuffer = node->GetDataBuffer();
  for(uint16_t i = 0; i < curNodeBuffer.size(); i++) {
    h.AddPriority(curNodeBuffer[i]->GetPriority());
    h.AddDataType(curNodeBuffer[i]->GetDataType());
    h.AddNodeId(curNodeBuffer[i]->GetNodeId());
    h.AddGenTime(curNodeBuffer[i]->GetGenTime());
    h.AddData(curNodeBuffer[i]->GetData());
  }
  p->AddHeader(h);

  Enqueue(p, node->GetParentMacAddress());
}
// GM-MAC
void AdhocWifiMac::ReceiveData(Ptr<Packet> p, const WifiMacHeader *hdr) {
  Ptr<Node> node = m_stationManager->GetNode();

  GmDataHeader destHeader;
  p->RemoveHeader(destHeader);
  vector<int64_t> tmpGenTime = destHeader.GetSensedGenTime();    
  vector<uint8_t> tmpPriority = destHeader.GetSensedPriority();  
  vector<uint8_t> tmpDataType = destHeader.GetSensedDataType();   
  vector<uint16_t> tmpNodeId = destHeader.GetSensedNodeId();   
  vector<uint16_t> tmpData = destHeader.GetSensedData();

  for(uint16_t i=0; i<tmpData.size(); i++) {
    Data* tmp = new Data();
    tmp->SetGenTime(tmpGenTime[i]);
    tmp->SetPriority(tmpPriority[i]);
    tmp->SetDataType(tmpDataType[i]);
    tmp->SetNodeId(tmpNodeId[i]);
    tmp->SetData(tmpData[i]);
    //
    if(node->GetId() == 0)    // arrival time is recorded when the data arrives at sink
      tmp->SetArrTime(Simulator::Now().GetTimeStep());

    node->AddData(tmp);
    node->AddDataAmount(12);
  }
  //NS_LOG_UNCOND("=======================" << node->GetId() << "==========================");
 
  if(node->GetId() == 0) {
    cout << "ReceiveData " << Simulator::Now() << " from " << hdr->GetAddr2() << " " << " Sink DataAmount : " << node->GetDataAmount() << '\n'; 
    // If there is any requirement for saving node information, use File I/O here.
      
    // sink buffer clear
    node->BufferClear();
  }
  else
    cout << "ReceiveData " << Simulator::Now() << " from " << hdr->GetAddr2() << " " << " Node " << hex << node->GetId()+1 << " DataAmount : " << node->GetDataAmount() << '\n';
}

void AdhocWifiMac::Sleep(void) {
  m_low->Sleep();
}

void AdhocWifiMac::IsSet(void) {
  Ptr<Node> node = m_stationManager->GetNode();
  if(node->GetGroupNumber() == -1) {
    SetStateType(NODE_SLEEP);
    Sleep();
  }
}
// GM-MAC
void AdhocWifiMac::Active(void) {
  Ptr<Node> node = m_stationManager->GetNode();
  Ptr<EnergySourceContainer> EnergySourceContainerOnNode = node->GetObject<EnergySourceContainer> ();
  Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (EnergySourceContainerOnNode->Get(0));

  m_activeNum++;    // if activeNum is up to periodicResettingThreshold, all nodes get to group number setting process.
  if(m_activeNum == periodicResettingThreshold) {
    //cout << Simulator::Now() << " Group Number Initailization " << hex << node->GetId()+1 <<  "\n";
    m_activeNum = 0;
    m_requestFailCount = 0;
    SetStateType(INIT);
    node->SetGroupNumber(-1);
    if(node->GetId() == 0) {    // if activated node is SINK, it starts to send initailization packet.
      Mac48Address m("ff:ff:ff:ff:ff:ff");
      Ptr<Packet> packet = Create<Packet> (0);
      //Enqueue(packet, m);
      Simulator::Schedule(MilliSeconds(5), &AdhocWifiMac::Enqueue, this, packet, m);
    }
    Simulator::Schedule(MilliSeconds(100), &AdhocWifiMac::IsSet, this);
  }
  if(GetStateType() != INIT) {    // sensor nodes sleep when it doesn't take any packets from other node except INIT state.
    if(m_requestFailCount == 4 || GetStateType() == NODE_SLEEP) { // When a sensor node couldn't take REPLY packet  4 times for REQUEST packet, it takes a sleep until periodic resetting.
      Sleep();
      void (AdhocWifiMac::*fp2)(void) = &AdhocWifiMac::Active;  // for periodic resetting
      m_activeId = Simulator::Schedule(MilliSeconds(node->GetFrameSize()), fp2, this);
      return;
    }
    void (AdhocWifiMac::*fp1)(void) = &AdhocWifiMac::Sleep;
    m_TAId = Simulator::Schedule(MilliSeconds(node->GetTA()), fp1, this);
  }

  void (AdhocWifiMac::*fp2)(void) = &AdhocWifiMac::Active;
  m_activeId = Simulator::Schedule(MilliSeconds(node->GetFrameSize()), fp2, this);

  m_phy->ResumeFromSleep(); //GM-MAC : activing

  Ptr<DeviceEnergyModel> basicRadioModelPtr = basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
  basicRadioModelPtr->SetAttribute("RxCurrentA", DoubleValue(0.056));
  basicRadioModelPtr->SetAttribute("TxCurrentA", DoubleValue(0.120));
  basicRadioModelPtr->SetAttribute("IdleCurrentA", DoubleValue(0.070));
  basicRadioModelPtr->SetAttribute("SleepCurrentA", DoubleValue(0.015));
  // DataTransmission
  // 1. the amount of data is over buffer threshold
  // 2. the node is sensor node, not sink
  // 3. the state is 
  if(node->GetDataAmount() >= node->GetBufferThreshold() && node->GetId() != 0 && GetStateType() == SET && m_requestFailCount < 4) {
    DataTransmission();
  }
  else if (GetStateType() == REQUEST) {   // If this node takes REQUEST packet from parent node at last time frame, it starts to select new parent node.
    Mac48Address m("ff:ff:ff:ff:ff:ff");
    Enqueue(Create<Packet> (0), m);
  }
}
// GM-MAC
void AdhocWifiMac::SelectParentNode(void) {
  Ptr<Node> node = m_stationManager->GetNode();
  if(m_receivedInfo.empty()) {
    m_requestFailCount++;
    cout << "id : " << hex << node->GetId()+1 << " Parent node resetting FAIL " << '\n';
    SetStateType(REQUEST);  // If regrouping is failed, it starts to send data.
    return ;
  }

  int16_t min = 9999;
  int minIndex = 0;
  for(uint16_t i=0;i<m_receivedInfo.size(); i++) {
    //NS_LOG_UNCOND(Simulator::Now() << " Collected REPLY packet " << m_receivedInfo[i].first << " " << m_receivedInfo[i].second);
    if(min > m_receivedInfo[i].second) {
      min = m_receivedInfo[i].second;
      minIndex = i;
    }
  }

  node->SetParentMacAddress(m_receivedInfo[minIndex].first);
  node->SetGroupNumber(m_receivedInfo[minIndex].second + 1);

  cout << "id : " << hex << node->GetId()+1 << " Parent node is changed to " << m_receivedInfo[minIndex].first << '\n';

  m_requestFailCount = 0;
  SetStateType(SET);
}
// GM-MAC
void
AdhocWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  Ptr<Node> node = m_stationManager->GetNode();
  //NS_LOG_UNCOND("AdhocwifiMac::Enqueue " << packet->GetSize() << " from " << m_low->GetAddress() << " to " << to);
  NS_LOG_FUNCTION (this << packet << to);
  if (m_stationManager->IsBrandNew (to))
    {
      //In ad hoc mode, we assume that every destination supports all
      //the rates we support.
      if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
        {
          m_stationManager->AddAllSupportedMcs (to);
        }
      if (GetHtSupported ())
        {
          m_stationManager->AddStationHtCapabilities (to, GetHtCapabilities ());
        }
      if (GetVhtSupported ())
        {
          m_stationManager->AddStationVhtCapabilities (to, GetVhtCapabilities ());
        }
      if (GetHeSupported ())
        {
          m_stationManager->AddStationHeCapabilities (to, GetHeCapabilities ());
        }
      m_stationManager->AddAllSupportedModes (to);
      m_stationManager->RecordDisassociated (to);
    }

  WifiMacHeader hdr;
  //NS_LOG_UNCOND("Mac48Address for advertising : " << to );

  //If we are not a QoS STA then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  //For now, a STA that supports QoS does not support non-QoS
  //associations, and vice versa. In future the STA model should fall
  //back to non-QoS if talking to a peer that is also non-QoS. At
  //that point there will need to be per-station QoS state maintained
  //by the association state machine, and consulted here.
  if (GetQosSupported ())
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      //Transmission of multiple frames in the same TXOP is not
      //supported for now
      hdr.SetQosTxopLimit (0);

      //Fill in the QoS control field in the MAC header
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which will
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetType (WIFI_MAC_DATA);
    }

  /******************************************/
    // GM-MAC
    //NS_LOG_UNCOND(node->GetId());
    if(node->GetNodeType() == SINK) {
      if (GetStateType() == INIT) {    // group number broadcasting from SINK

        // Node State
        node->SetNodeState(NODE_DATA);
        node->SetGroupNumber(0);
        // MAC State
        SetStateType(SET); //GM-MAC : 'SET' : status for data transmission
        // Header Configuration
        hdr.SetCtrlPktType(WIFI_MAC_GM_INIT);
        hdr.SetFrameSize(node->GetFrameSize());
        hdr.SetTA(node->GetTA());
        // Activated after frame size
        m_activeId.Cancel();  // cancel old schedule
        void (AdhocWifiMac::*fp)(void) = &AdhocWifiMac::Active;
        m_activeId = Simulator::Schedule(MilliSeconds(node->GetFrameSize()), fp, this); // create new schedule
      }
      else if (GetStateType() == SET) {
        hdr.SetCtrlPktType(WIFI_MAC_GM_DATA);
      }
      else if(GetStateType() == REPLY) {
        //NS_LOG_UNCOND("AdhocWifiMac::Enqueue::REPLY");
        SetStateType(SET);
        hdr.SetCtrlPktType(WIFI_MAC_GM_REPLY);
      }
      hdr.SetNodeType(SINK);
    }
    else if(node->GetNodeType() == SENSOR) {
      if(GetStateType() == INIT) {    // group number broadcasting from SENSOR
        // MAC State
        SetStateType(SET);
        //Header Configuration
        hdr.SetCtrlPktType(WIFI_MAC_GM_INIT);
        hdr.SetFrameSize(node->GetFrameSize());
        hdr.SetTA(node->GetTA());
        // Actived after frame size
        m_activeId.Cancel();  // cancel old schedule
        void (AdhocWifiMac::*fp)(void) = &AdhocWifiMac::Active;
        m_activeId = Simulator::Schedule(MilliSeconds(node->GetFrameSize()), fp, this); // create new schedule
      }
      else if(GetStateType() == SET) {    // data transmission from SENSOR
        hdr.SetCtrlPktType (WIFI_MAC_GM_DATA);
        //NS_LOG_UNCOND ("Data Transmission");
        //NS_LOG_UNCOND (Simulator::Now() << " / from : " << m_low->GetAddress() <<" ---> to : " << to);
      }
      else if (GetStateType() == REQUEST) {
        NS_LOG_UNCOND(Simulator::Now() << " AdhocWifiMac::Enqueue::REQUEST " << node->GetId()+1);
        hdr.SetCtrlPktType(WIFI_MAC_GM_REQUEST);

        void (AdhocWifiMac::*fp)(void) = &AdhocWifiMac::SelectParentNode;
        Simulator::Schedule(MilliSeconds(50), fp, this);
      }
      else if(GetStateType() == REPLY) {
        //NS_LOG_UNCOND(Simulator::Now() << " AdhocWifiMac::Enqueue::REPLY from " << node->GetId() + 1 << " to " << to);
        SetStateType(SET);
        hdr.SetCtrlPktType(WIFI_MAC_GM_REPLY);
      }
      hdr.SetNodeType(SENSOR);
    }
    /******************************************/ 

  if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
    {
      hdr.SetNoOrder (); // explicitly set to 0 for the time being since HT/VHT/HE control field is not yet implemented (set it to 1 when implemented)
    }
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_low->GetAddress ());
  //NS_LOG_UNCOND("MAC from : " << m_low->GetAddress());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  // GM-MAC
  hdr.SetGroupNumber(node->GetGroupNumber());

  if (GetQosSupported ())
    {
      //Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
    }
  else
    {
      m_txop->Queue (packet, hdr);
    }
}

void
AdhocWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  //The approach taken here is that, from the point of view of a STA
  //in IBSS mode, the link is always up, so we immediately invoke the
  //callback if one is set
  linkUp ();
}

void
AdhocWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  //NS_LOG_UNCOND("AdhocWifiMac::Receive");
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();
  if (m_stationManager->IsBrandNew (from))
    {
      //In ad hoc mode, we assume that every destination supports all
      //the rates we support.
      if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
        {
          m_stationManager->AddAllSupportedMcs (from);
          m_stationManager->AddStationHtCapabilities (from, GetHtCapabilities ());
        }
      if (GetHtSupported ())
        {
          m_stationManager->AddStationHtCapabilities (from, GetHtCapabilities ());
        }
      if (GetVhtSupported ())
        {
          m_stationManager->AddStationVhtCapabilities (from, GetVhtCapabilities ());
        }
      if (GetHeSupported ())
        {
          m_stationManager->AddStationHeCapabilities (from, GetHeCapabilities ());
        }
      m_stationManager->AddAllSupportedModes (from);
      m_stationManager->RecordDisassociated (from);
    }
  if (hdr->IsData ())
    {
      /******************************************/
      // GM-MAC
      Ptr<Node> node = m_stationManager->GetNode();

      if(hdr->GetCtrlPktType() == WIFI_MAC_GM_INIT) { //GM-MAC : 'WIFI_MAC_GM_INIT' : initializing format
        //NS_LOG_UNCOND("WIFI_MAC_GM_INIT");
        int16_t currentGn = node->GetGroupNumber();
        int16_t receivedGn = hdr->GetGroupNumber();
        if(currentGn == -1 || currentGn > receivedGn + 1) {
          node->SetGroupNumber(receivedGn + 1);
          SetStateType(INIT);
          node->SetParentMacAddress(from);

          //node->SetNodeState(NODE_INIT);
          node->SetFrameSize(hdr->GetFrameSize());
          node->SetTA(hdr->GetTA());
          //NS_LOG_UNCOND("WIFI_MAC_GM_INIT");
          cout << "from : " << from <<" ---> to : " << m_low->GetAddress()  <<  " / Group number is changed to : " << receivedGn + 1 << '\n';
          // Broadcasting
          Mac48Address m("ff:ff:ff:ff:ff:ff");//GM-MAC : MAC address for broadcast
          Enqueue(Create<Packet> (0), m);
          return;
        }
        else {
          // ignoring initialization packet
          //NS_LOG_UNCOND("WIFI_MAC_GM_INIT");
          NS_LOG_UNCOND("from : " << from <<" ---> to : " << m_low->GetAddress()  << " / initialization packet is ignored");
          return;
        }
      }
      else if (hdr->GetCtrlPktType() == WIFI_MAC_GM_DATA) { //GM-MAC : 'WIFI_MAC_GM_DATA' : data transmission format
        ReceiveData(packet, hdr);
        return ;
      }
      else if(hdr->GetCtrlPktType() == WIFI_MAC_GM_REQUEST) { //GM-MAC : 'WIFI_MAC_GM_REQUEST' : request format
        //NS_LOG_UNCOND("AdhocWifiMac::Receive::WIFI_MAC_GM_REQUEST");
        if(from != node->GetParentMacAddress() && GetStateType() != REQUEST) {
          //NS_LOG_UNCOND(Simulator::Now() << " AdhocWifiMac::Receive::WIFI_MAC_GM_REQUEST::NOTPARENT");
          SetStateType(REPLY);
          Enqueue(Create<Packet> (0), from);
        }
        else if (from == node->GetParentMacAddress()) {  
          // If this node takes REQUEST packet from its parent node,
          // it takes a sleep mode and starts to send REQUEST packet at next time frame.
          //NS_LOG_UNCOND(Simulator::Now() << " AdhocWifiMac::Receive::WIFI_MAC_GM_REQUEST::FROMPARENT");
          SetStateType(REQUEST);
          Sleep();
        }
        return;
      }
      else if(hdr->GetCtrlPktType() == WIFI_MAC_GM_REPLY) { //GM-MAC : 'WIFI_MAC_GM_REPLY' : reply format
        if(GetStateType() == REQUEST) {
          pair<Mac48Address,int16_t> tmp = make_pair(from, hdr->GetGroupNumber());
          m_receivedInfo.push_back(tmp);
        }
        return;
      }
      /******************************************/ 
      if (hdr->IsQosData () && hdr->IsQosAmsdu ())
        {
          NS_LOG_DEBUG ("Received A-MSDU from" << from);
          DeaggregateAmsduAndForward (packet, hdr);
        }
      else
        {
          //NS_LOG_UNCOND("from : " << from <<" ---> to : " << m_low->GetAddress());
          ForwardUp (packet, from, to);
        }
      return;
    }

  //Invoke the receive handler of our parent class to deal with any
  //other frames. Specifically, this will handle Block Ack-related
  //Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

} //namespace ns3
