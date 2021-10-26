#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("scratch-simulator");

int main (int argc, char *argv[]) {
  SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  SeedManager::SetRun (1);  // Changes run number from default of 1 to 7
  double simulationTime = 2; // Seconds
  double distance = 1.0; // Meters
  uint32_t nWifi = 3; // Number of stations
  uint32_t MCS = 5; // Number of stations
  uint32_t txPower = 15; // Number of stations
  bool agregation = false; // Allow or not the packet agregation
  std::string trafficDirection = "upstream";
  uint32_t payloadSize = 1472; 
  std::string dataRate = "10"; 

  CommandLine cmd (__FILE__);
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("MCS", "Simulation time in seconds", MCS);
  cmd.AddValue ("txPower", "Simulation time in seconds", txPower);
  cmd.AddValue ("payloadSize", "Simulation time in seconds", payloadSize);
  cmd.AddValue ("dataRate", "Simulation time in seconds", dataRate);
  cmd.AddValue("nWifi", "Number of all stations", nWifi);
  cmd.AddValue("trafficDirection", "Traffic Direction", trafficDirection);
  cmd.Parse (argc,argv);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  WifiMacHelper mac;
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211ac);

  int channelWidth = 80; // BW Channel Width
  int sgi = 0; // Indicates whether Short Guard Interval is enabled or not (SGI==1 <==> GI=400ns)

  std::ostringstream oss;
  oss << "VhtMcs" << MCS;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
                                "ControlMode", StringValue (oss.str ()));

  Ssid ssid = Ssid ("ns3-80211ac");

  // Installing phy & mac layers on the overloading stations
  mac.SetType ("ns3::StaWifiMac",
              "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  // Installing phy & mac layers on the AP
  mac.SetType ("ns3::ApWifiMac",   
              "EnableBeaconJitter", BooleanValue (false),
              "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

  // Set channel width
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (channelWidth));

  // Set guard interval
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (sgi));

  // Setting stations' positions
  MobilityHelper mobility;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  for (uint32_t i = 0; i < nWifi; i++) {
      positionAlloc->Add (Vector (distance, 0.0, 0.0));
  }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  Ptr<ListPositionAllocator> positionAllocAp = CreateObject<ListPositionAllocator> ();
  positionAllocAp->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAllocAp);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  // Setting IP addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.0.0");
  Ipv4InterfaceContainer ApInterface;
  ApInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer StaInterface;
  StaInterface = address.Assign (staDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  if (agregation == false) {
    // Disable A-MPDU & A-MSDU in AP
    Ptr<NetDevice> dev = wifiApNode.Get(0)-> GetDevice(0);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (0));
  }

  // Set txPower
  Ptr<WifiPhy> phy_tx = dynamic_cast<WifiNetDevice*>(GetPointer((staDevices.Get(0))))->GetPhy();
  phy_tx->SetTxPowerEnd(txPower);
  phy_tx->SetTxPowerStart(txPower);

  //phy_tx->SetRxSensitivity(sensibility);
  //phy_tx->SetCcaEdThreshold(sensibility);

  /* Setting applications */
  ApplicationContainer sourceApplications, sinkApplications;
  uint32_t portNumber = 9;

  if (trafficDirection == "upstream") {
    auto ipv4 = wifiApNode.Get (0)->GetObject<Ipv4> ();
    const auto address = ipv4->GetAddress (1, 0).GetLocal ();
    InetSocketAddress sinkSocket (address, portNumber);
      
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
    sinkApplications.Add (packetSinkHelper.Install (wifiApNode.Get (0)));
    for (uint32_t index = 0; index < nWifi; ++index) {
      if (agregation == false) {
        // Disable A-MPDU & A-MSDU in each station
        Ptr<NetDevice> dev = wifiStaNodes.Get (index)->GetDevice (0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (0));
      }
 
      // UDP Client application to be installed in the stations
      OnOffHelper server ("ns3::UdpSocketFactory", sinkSocket);
      server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
      server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate+"Mbps")));

      sourceApplications.Add (server.Install (wifiStaNodes.Get (index)));
    }
  }
  else {
    for (uint32_t index = 0; index < nWifi; ++index) {
      if (agregation == false) {
        // Disable A-MPDU & A-MSDU in each station
        Ptr<NetDevice> dev = wifiStaNodes.Get (index)->GetDevice (0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
        wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (0));
      }

      auto ipv4 = wifiStaNodes.Get (index)->GetObject<Ipv4> ();
      const auto address = ipv4->GetAddress (1, 0).GetLocal ();
      InetSocketAddress sinkSocket (address, portNumber);
      
      // UDP Client application to be installed in the stations
      OnOffHelper server ("ns3::UdpSocketFactory", sinkSocket);
      server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
      server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate+"Mbps")));

      sourceApplications.Add(server.Install (wifiApNode.Get(0)));
      
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
      sinkApplications.Add(packetSinkHelper.Install (wifiStaNodes.Get (index)));      
    }
  }

  // Start the UDP Client & Server applications
  sinkApplications.Start (Seconds (0.0));
  sinkApplications.Stop (Seconds (simulationTime + 1));
  sourceApplications.Start (Seconds (1.0));
  sourceApplications.Stop (Seconds (simulationTime + 1));

  AsciiTraceHelper ascii;

  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  std::string s = "energy/serrano/"+ dataRate+"-"+std::to_string(MCS)+"-"+std::to_string(txPower)+"-"+std::to_string(payloadSize);
  phy.EnableAsciiAll (ascii.CreateFileStream(s+".tr"));
  //phy.EnablePcap (s+".pcap", apDevice.Get(0), false, true);

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

  double throughput = 0;
  for (uint32_t index = 0; index < sinkApplications.GetN (); ++index) {
    uint64_t totalPacketsThrough = DynamicCast<PacketSink> (sinkApplications.Get (index))->GetTotalRx ();
    throughput += ((totalPacketsThrough * 8) / ((simulationTime) * 1000000.0)); //Mbit/s
  }

  Simulator::Destroy ();

  std::cout << nWifi << " " << throughput << std::endl;
  
  return 0;
}