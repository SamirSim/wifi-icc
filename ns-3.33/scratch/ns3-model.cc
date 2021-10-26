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
#include "ns3/energy-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ns3-model");

 void
 RemainingEnergy (double oldValue, double remainingEnergy)
 {
   NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                  << "s Current remaining energy = " << remainingEnergy << "J");
 }
 
 void
 TotalEnergy (double oldValue, double totalEnergy)
 {
   NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                  << "s Total energy consumed by radio = " << totalEnergy << "J");
 }

int main (int argc, char *argv[]) {
  SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  SeedManager::SetRun (1);  // Changes run number from default of 1 to 7
  double simulationTime = 7; // Seconds
  double distance = 1.0; // Meters
  uint32_t nWifi = 3; // Number of stations
  uint32_t MCS = 0; // Number of stations
  uint32_t txPower = 15; // Number of stations
  bool agregation = false; // Allow or not the packet agregation
  std::string trafficDirection = "upstream";
  uint32_t payloadSize = 1472; 
  std::string dataRate = "5"; 

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

  //LogComponentEnable ("EnergySource", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("BasicEnergySource", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("DeviceEnergyModel", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("WifiRadioEnergyModel", LOG_LEVEL_ALL);

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

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

  int channelWidth = 80; // BW Channel Width in MHz
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

   /***************************************************************************/
   /* energy source */
   BasicEnergySourceHelper basicSourceHelper;
   //basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));
   //RvBatteryModelHelper rvModelHelper;
   // configure energy source
   // install source
   EnergySourceContainer sources = basicSourceHelper.Install(wifiStaNodes);
   EnergySourceContainer source_ap = basicSourceHelper.Install(wifiApNode);
   /* device energy model */
   WifiRadioEnergyModelHelper radioEnergyHelper;
   // configure radio energy model
   //radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
   // install device model
   DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (staDevices, sources);
   DeviceEnergyModelContainer apModel = radioEnergyHelper.Install (apDevice, source_ap);

   /***************************************************************************/
   // all sources are connected to node 1
   // energy source
   for (uint32_t index = 0; index < nWifi; ++index) {
    Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (sources.Get (index));
    //basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
    // device energy model
    Ptr<DeviceEnergyModel> basicRadioModelPtr =
      basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
    NS_ASSERT (basicRadioModelPtr != NULL);
    //basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
   }

   Ptr<BasicEnergySource> basicSourcePtrAp = DynamicCast<BasicEnergySource> (source_ap.Get (0));
   //basicSourcePtrAp->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
   // device energy model
   Ptr<DeviceEnergyModel> basicRadioModelPtrAp =
     basicSourcePtrAp->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
   NS_ASSERT (basicRadioModelPtrAp != NULL);
   //basicRadioModelPtrAp->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
   /***************************************************************************/

  AsciiTraceHelper ascii;

  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  //std::string s = "../../Data/wifi/overload/throughput/" + trafficDirection + "/ac/traces/" + std::to_string(nWifi);
  //std::string s = "energy/ac/"+ dataRate+"-"+std::to_string(MCS)+"-"+std::to_string(txPower);
  //phy.EnableAsciiAll (ascii.CreateFileStream(s+".tr"));
  //phy.EnablePcap (s+".pcap", apDevice.Get(0), false, true);

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();
  
  for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++)
     {
       double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
       NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                      << "s) Total energy consumed by radio (Station) = " << energyConsumed << "J");
     }
     for (DeviceEnergyModelContainer::Iterator iter = apModel.Begin (); iter != apModel.End (); iter ++)
     {
       double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
       NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                      << "s) Total energy consumed by radio (AP) = " << energyConsumed << "J");
     }
  
  double throughput = 0;
  for (uint32_t index = 0; index < sinkApplications.GetN (); ++index) {
    uint64_t totalPacketsThrough = DynamicCast<PacketSink> (sinkApplications.Get (index))->GetTotalRx ();
    throughput += totalPacketsThrough ; //Number of packets
  }

  Simulator::Destroy ();
    
  return 0;
}