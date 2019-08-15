/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/energy-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"

#include "ns3/ipv4-route.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/gm-data-header.h"
#include "ns3/data.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/wifi-net-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");

#define numNodes 50
#define genDataDuration 5   // Seconds
#define stopTime 600   // Seconds
#define frameSize 610 // T-MAC frame size (ms)
#define TA 15 // T-MAC TA (ms)
#define energyTrackingTime 30 // Energy Tracking per time (sec)

Ptr<Socket> recvSink[numNodes];
Ptr<Socket> source[numNodes];
NodeContainer c;
Ipv4InterfaceContainer ipv4Inter;
vector<Ptr<BasicEnergySource> > basicSourcePtr;

void ReconfigSocket (Ptr<Node> node) {
  //NS_LOG_UNCOND(node->GetId() << " : Reconfig");
  int16_t x = node->GetId();

  source[x]->Close();     // close socket that aleady exist
  
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  source[x] = Socket::CreateSocket (node, tid);    // node , sender
  InetSocketAddress remote = InetSocketAddress (node->GetParentIpv4Address(), 80);
  source[x]->SetAllowBroadcast (true);
  source[x]->Connect (remote);

  Ptr<Ipv4> ip = node->GetObject<Ipv4> ();
  Ipv4Address src = ip->GetAddress(1, 0).GetLocal();
  NS_LOG_UNCOND("Child Node : " << src << " / Parent Node : " << node->GetParentIpv4Address());
}

//GM-MAC : group number initialization(trigger)
static void Initialization (Ptr<Socket> socket)
{
  Ptr<Node> sink = c.Get(0);

  sink->SetFrameSize(frameSize);
  sink->SetTA(TA);

  sink->SetNodeState(NODE_INIT);    // node state for Ipv4L3Protocol

  Ptr<NetDevice> device = sink->GetDevice(0);
  Ptr<AdhocWifiMac> mac = device->GetObject<WifiNetDevice>()->GetMac()->GetObject<AdhocWifiMac>();
  mac->SetStateType(INIT);  // MAC state for AdhocWifiMac(MAC High)//GM-MAC :  for resetting 
  socket->Send (Create<Packet> (0)); //GM-MAC : go to enqueue() of adhoc-wifi-mac.cc
}

//GM-MAC : 
void SenseData (void) {
  //NS_LOG_UNCOND(Simulator::Now());
  for(int i=1;i<numNodes;i++) {   // except for sink
    Data* curData = new Data();
    curData->SetPriority (0);
    curData->SetDataType (0);
    curData->SetNodeId (c.Get(i)->GetId());//GM-MAC : me
    curData->SetGenTime (Simulator::Now().GetTimeStep());
    curData->SetData (0);//GM-MAC : real data

    c.Get(i)->AddData (curData);
    c.Get(i)->AddDataAmount (12);//GM-MAC : if updating GM-Data-Header, edit this
    //NS_LOG_UNCOND(c.Get(i)->GetId() << " : " << c.Get(i)->GetDataAmount());
  }

  //NS_LOG_UNCOND(Simulator::Now());
  Simulator::Schedule (Seconds(genDataDuration), &SenseData);
}

/*
void
CurrentDataAmount (Ptr<Node> node, double oldValue, double dataAmount)
{
  /
  if(dataAmount >= node->GetBufferThreshold()) {
    //NS_LOG_UNCOND(node->GetId());
    DataTransmission(node);
  }
}
*/
/// Trace function for remaining energy at node.
void
RemainingEnergy (Ptr<Node> node, double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Current remaining energy = " << remainingEnergy << "J");
}

/// Trace function for total energy consumption at node.
void
TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Total energy consumed by radio = " << totalEnergy << "J");
}

void EnergyTracking (void) {
  ofstream fout;
  fout.open("energy.txt", ios::ate|ios::app);
  for(int i=1;i<numNodes;i++) {
    Ptr<BasicEnergySource> cur = basicSourcePtr[i];
    // File I/O for current node
    fout << cur->GetRemainingEnergy() << '\t';
  }
  fout << '\n';
  fout.close();
  Simulator::Schedule(Seconds(energyTrackingTime), &EnergyTracking);
}

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double Prss = -80;            // dBm
  uint32_t PpacketSize = 200;   // bytes
  bool verbose = false;

  // simulation parameters
  uint32_t numPackets = 1 ;  // number of packets to send
  double startTime = 0.0;       // seconds
  double distanceToRx = 100.0;  // meters
  /*
   * This is a magic number used to set the transmit power, based on other
   * configuration.
   */
  double offset = 81;

  CommandLine cmd;
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("Prss", "Intended primary RSS (dBm)", Prss);
  cmd.AddValue ("PpacketSize", "size of application packet sent", PpacketSize);
  cmd.AddValue ("numPackets", "Total number of packets to send", numPackets);
  cmd.AddValue ("startTime", "Simulation start time", startTime);
  cmd.AddValue ("distanceToRx", "X-Axis distance between nodes", distanceToRx);
  cmd.AddValue ("verbose", "Turn on all device log components", verbose);
  cmd.Parse (argc, argv);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  c.Create (numNodes);

/*
  Ptr<Node> nodePtr[numNodes];
  for(int i=1;i<numNodes;i++) {
    nodePtr[i] = DynamicCast<Node> (c.Get (i));
    nodePtr[i]->TraceConnectWithoutContext ("CurrentDataAmount", MakeBoundCallback (&CurrentDataAmount, c.Get(i)));
  }
*/
  

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  /** Wifi PHY **/
  /***************************************************************************/
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.Set ("RxGain", DoubleValue (-10));
  wifiPhy.Set ("TxGain", DoubleValue (offset + Prss));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (0.0));
  /***************************************************************************/

  /** wifi channel **/
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  // create wifi channel
  Ptr<YansWifiChannel> wifiChannelPtr = wifiChannel.Create ();
  wifiPhy.SetChannel (wifiChannelPtr);

  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode),
                                "RtsCtsThreshold", UintegerValue(32));    //Disable RTS for REPLY packet (value = 32)

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

//GM-MAC : add mobility to sink 
  MobilityHelper sinkMobility;
  Ptr<ListPositionAllocator> sinkPositionAlloc = CreateObject<ListPositionAllocator> ();
  sinkPositionAlloc->Add (Vector (5000.0, 5000.0, 0));
  sinkMobility.SetPositionAllocator (sinkPositionAlloc);
  sinkMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  sinkMobility.Install (c.Get(0));//GM-MAC : if there is another sinks, install that.
///////////////////////////////

//GM-MAC : add mobility to sensor
  MobilityHelper sensorMobility;
  sensorMobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("5000.0"),
                                 "Y", StringValue ("5000.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=3000]"));//GM-MAC : random position allocator, standard is (5000,5000)

  sensorMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("1s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|10000|0|10000"));
  for(int i=1;i<numNodes;i++) {
    sensorMobility.Install(c.Get(i));
  }

   /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (10000));//GM-MAC : 100J
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (c); //GM-MAC : installing 100J to each sensor
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, sources);
  /***************************************************************************/

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4Inter = ipv4.Assign (devices);

  Ipv4InterfaceContainer::Iterator f = ipv4Inter.Begin();
  f++;
  for(uint32_t i = 1; i < ipv4Inter.GetN(); i++) {
    Ptr<Ipv4L3Protocol> protocol = f->first->GetObject<Ipv4L3Protocol> ();
    protocol->SetCreateSocketCallback(MakeCallback (&ReconfigSocket));
    f++;
  }

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  for(int x=0;x<numNodes;x++) {
    recvSink[x] = Socket::CreateSocket (c.Get (x), tid);  // node , receiver
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
    recvSink[x]->Bind (local);
  }
  for(int x=0;x<numNodes;x++) {
    source[x] = Socket::CreateSocket (c.Get(x), tid);    // sink , sender
    InetSocketAddress remote = InetSocketAddress (Ipv4Address::GetBroadcast(), 80);
    source[x]->SetAllowBroadcast (true);
    source[x]->Connect (remote);
  }

    /** connect trace sources **/
  /***************************************************************************/
  // all sources are connected to node 1
  // energy source
  for(int i=0;i<numNodes;i++) {
    basicSourcePtr.push_back(DynamicCast<BasicEnergySource> (sources.Get (i)));
  }
  Simulator::Schedule(Seconds(energyTrackingTime), &EnergyTracking);

  //basicSourcePtr0->TraceConnectWithoutContext ("RemainingEnergy", MakeBoundCallback (&RemainingEnergy, c.Get(0)));
  // device energy model
  /*
  Ptr<DeviceEnergyModel> basicRadioModelPtr = basicSourcePtr0->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
  NS_ASSERT (basicRadioModelPtr != NULL);
  basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption" ,MakeCallback (&TotalEnergy));
  */
  /***************************************************************************/

  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  // Output what we are doing
  //NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent with receiver rss " << rss );

//GM-MAC : using below code only once for initializing group number(socket)
  Simulator::ScheduleWithContext (source[0]->GetNode ()->GetId (),
                                  Seconds (1.0), &Initialization,
                                  source[0]);
  Simulator::Schedule (Seconds(genDataDuration), &SenseData);//GM-MAC : getDataDuration : how often sensing data

  //Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
