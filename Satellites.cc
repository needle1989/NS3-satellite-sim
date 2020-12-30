 #include <iostream>
 #include <fstream>
#include <string>
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/netanim-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/point-to-point-layout-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/flow-monitor-helper.h"
#include "ns3/tcp-header.h"
 #include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"


 using namespace ns3;


//NetHelper

void NetHelper (uint16_t sinkPort,NodeContainer nodes,Ipv4InterfaceContainer ixix,int num,int receiver,int sender,std::string rate,int cwnd)
{
   Address sinkAddress (InetSocketAddress (ixix.GetAddress (num), sinkPort));
   PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
   
      AddressValue remoteAddress (InetSocketAddress (ixix.GetAddress (num), sinkPort));
      BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
      ftp.SetAttribute ("Remote", remoteAddress);
      ftp.SetAttribute ("MaxBytes", UintegerValue (6 * 1000000));

      ApplicationContainer sourceApp = ftp.Install (nodes.Get (sender));
      sourceApp.Start (Seconds (0));
      sourceApp.Stop (Seconds (20));
   
   
   
   ApplicationContainer sinkApps= packetSinkHelper.Install (nodes.Get (receiver));
   sinkApps.Start (Seconds (0.));
   sinkApps.Stop (Seconds (20.));
   Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (sender), TcpSocketFactory::GetTypeId ());
   
     OnOffHelper onoff ("ns3::TcpSocketFactory",
                     Address (InetSocketAddress (ixix.GetAddress (num), sinkPort)));
  onoff.SetConstantRate (DataRate (rate));
  ApplicationContainer apps = onoff.Install (nodes.Get (sender));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (20.0));
}

//Tracer
static Ptr<OutputStreamWrapper> nextRxStream;

static void
NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
{
  NS_UNUSED (old);
  *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
}


static void
TraceNextRx (std::string &next_rx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextRxStream = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence", MakeCallback (&NextRxTracer));
}

//main
 int main (int argc, char *argv[])
 {
 
   //a dumbbell-like high-latency hybrid network environment 
   NodeContainer nodes;
   nodes.Create (4);
   NodeContainer h0r0 = NodeContainer (nodes.Get (0), nodes.Get (2)); 
   NodeContainer h1r1 = NodeContainer (nodes.Get (1), nodes.Get (3)); 
   NodeContainer r0r1 = NodeContainer (nodes.Get (2), nodes.Get (3)); 


   InternetStackHelper internet;
   internet.Install (nodes);
   PointToPointHelper p2p;
   p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
   p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
   PointToPointHelper p2psatellite;
   p2psatellite.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
   p2psatellite.SetChannelAttribute ("Delay", StringValue ("10ms"));
   NetDeviceContainer dh0r0 = p2p.Install (h0r0);
   NetDeviceContainer dh1r1 = p2p.Install (h1r1);
   NetDeviceContainer dr0r1 = p2psatellite.Install (r0r1);
   
   Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate",DoubleValue(0.05),"ErrorUnit",EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
   dr0r1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
   
   
   //app
   Ipv4AddressHelper IPh0r0 = Ipv4AddressHelper("15.0.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh1r1 = Ipv4AddressHelper("15.1.0.0", "255.255.255.0");
   Ipv4AddressHelper IPr0r1 = Ipv4AddressHelper("15.2.0.0", "255.255.255.0");
            
   Ipv4InterfaceContainer ih0r0 = IPh0r0.Assign (dh0r0);
   Ipv4InterfaceContainer ih1r1 = IPh1r1.Assign (dh1r1);
   Ipv4InterfaceContainer ir0r1 = IPr0r1.Assign (dr0r1);
   
   Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

   Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
    
   NetHelper (8080,nodes,ih1r1,0,1,0,"2Mbps",15000);   
   
   
   //animation
   MobilityHelper mobility;   
   mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
   				 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType" ,StringValue ("RowFirst"));
   mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   mobility.Install (nodes);
   AnimationInterface anim ("satellite.xml");
   anim.SetConstantPosition(nodes.Get(0),5,10);
   anim.SetConstantPosition(nodes.Get(1),20,10);
   anim.SetConstantPosition(nodes.Get(2),10,10);
   anim.SetConstantPosition(nodes.Get(3),15,10);

//tracing
   bool tracing=true;  
   bool pcap = true;
   bool flow_monitor=true;
   std::string prefix_file_name = "Satellites";
     if (tracing)
    {
      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
                                            std::ios::out);
      internet.EnableAsciiIpv4All (ascii_wrap);


      Simulator::Schedule (Seconds (1), &TraceNextRx, prefix_file_name + "-next-rx.data");
    }

  if (pcap)
    {
      p2p.EnablePcapAll (prefix_file_name, true);
      p2psatellite.EnablePcapAll (prefix_file_name, true);
    }

  // Flow monitor
  FlowMonitorHelper flowHelper;
  if (flow_monitor)
    {
      flowHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (20));
  Simulator::Run ();

  if (flow_monitor)
    {
      flowHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", true, true);
    }

   Simulator::Destroy ();
   return 0;
}
