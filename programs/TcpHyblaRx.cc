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
      sourceApp.Stop (Seconds (100));
   
   
   
   ApplicationContainer sinkApps= packetSinkHelper.Install (nodes.Get (receiver));
   sinkApps.Start (Seconds (0.));
   sinkApps.Stop (Seconds (100.));
   Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (sender), TcpSocketFactory::GetTypeId ());
      //Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
   
     OnOffHelper onoff ("ns3::TcpSocketFactory",
                     Address (InetSocketAddress (ixix.GetAddress (num), sinkPort)));
  onoff.SetConstantRate (DataRate (rate));
  ApplicationContainer apps = onoff.Install (nodes.Get (sender));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (100.0));
  
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
  Config::ConnectWithoutContext ("/NodeList/8/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence", MakeCallback (&NextRxTracer));
}

//main
 int main (int argc, char *argv[])
 {
 
   //a dumbbell-like high-latency hybrid network environment 
   NodeContainer nodes;
   nodes.Create (11);
   NodeContainer h0r0 = NodeContainer (nodes.Get (0), nodes.Get (9)); 
   NodeContainer h1r0 = NodeContainer (nodes.Get (1), nodes.Get (9)); 
   NodeContainer h2r0 = NodeContainer (nodes.Get (2), nodes.Get (9)); 
   NodeContainer h3r0 = NodeContainer (nodes.Get (3), nodes.Get (9)); 
   NodeContainer h4r1 = NodeContainer (nodes.Get (4), nodes.Get (10));
   NodeContainer h5r1 = NodeContainer (nodes.Get (5), nodes.Get (10)); 
   NodeContainer h6r1 = NodeContainer (nodes.Get (6), nodes.Get (10)); 
   NodeContainer h7r1 = NodeContainer (nodes.Get (7), nodes.Get (10)); 
   NodeContainer h7h8 = NodeContainer (nodes.Get (7), nodes.Get (8)); 
   NodeContainer r0r1 = NodeContainer (nodes.Get (9), nodes.Get (10)); 
   InternetStackHelper internet;
   internet.Install (nodes);
   PointToPointHelper p2p;
   p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
   p2p.SetChannelAttribute ("Delay", StringValue ("2.5ms"));
   PointToPointHelper p2pbottle;
   p2pbottle.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
   p2pbottle.SetChannelAttribute ("Delay", StringValue ("7.5ms"));
   PointToPointHelper p2pdelay;
   p2pdelay.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
   p2pdelay.SetChannelAttribute ("Delay", StringValue ("50ms"));
   NetDeviceContainer dh0r0 = p2p.Install (h0r0);
   NetDeviceContainer dh1r0 = p2p.Install (h1r0);
   NetDeviceContainer dh2r0 = p2p.Install (h2r0);
   NetDeviceContainer dh3r0 = p2p.Install (h3r0);
   NetDeviceContainer dh4r1 = p2p.Install (h4r1);
   NetDeviceContainer dh5r1 = p2p.Install (h5r1);
   NetDeviceContainer dh6r1 = p2p.Install (h6r1);
   NetDeviceContainer dh7r1 = p2pdelay.Install (h7r1);
   NetDeviceContainer dh7h8 = p2pdelay.Install (h7h8);
   NetDeviceContainer dr0r1 = p2pbottle.Install (r0r1);
   
   
   
   //app
   Ipv4AddressHelper IPh0r0 = Ipv4AddressHelper("15.0.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh1r0 = Ipv4AddressHelper("15.1.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh2r0 = Ipv4AddressHelper("15.2.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh3r0 = Ipv4AddressHelper("15.3.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh4r1 = Ipv4AddressHelper("15.4.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh5r1 = Ipv4AddressHelper("15.5.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh6r1 = Ipv4AddressHelper("15.6.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh7r1 = Ipv4AddressHelper("15.7.0.0", "255.255.255.0");
   Ipv4AddressHelper IPh7h8 = Ipv4AddressHelper("15.8.0.0", "255.255.255.0");
   Ipv4AddressHelper IPr0r1 = Ipv4AddressHelper("15.9.0.0", "255.255.255.0");
            
   Ipv4InterfaceContainer ih0r0 = IPh0r0.Assign (dh0r0);
   Ipv4InterfaceContainer ih1r0 = IPh1r0.Assign (dh1r0);
   Ipv4InterfaceContainer ih2r0 = IPh2r0.Assign (dh2r0);
   Ipv4InterfaceContainer ih3r0 = IPh3r0.Assign (dh3r0);
   Ipv4InterfaceContainer ih4r1 = IPh4r1.Assign (dh4r1);
   Ipv4InterfaceContainer ih5r1 = IPh5r1.Assign (dh5r1);
   Ipv4InterfaceContainer ih6r1 = IPh6r1.Assign (dh6r1);
   Ipv4InterfaceContainer ih7r1 = IPh7r1.Assign (dh7r1);
   Ipv4InterfaceContainer ih7h8 = IPh7h8.Assign (dh7h8);
   Ipv4InterfaceContainer ir0r1 = IPr0r1.Assign (dr0r1);
   
   Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

   Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
    
   NetHelper (8080,nodes,ih7h8,1,8,0,"1Mbps",15000);   
   NetHelper (8081,nodes,ih6r1,0,6,1,"1Mbps",15000);  
   NetHelper (8082,nodes,ih5r1,0,5,2,"1Mbps",15000);  
   NetHelper (8083,nodes,ih4r1,0,4,3,"1Mbps",15000);  
   
   
   
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
   AnimationInterface anim ("hybla.xml");
   anim.SetConstantPosition(nodes.Get(0),15,14);
   anim.SetConstantPosition(nodes.Get(1),15,24);
   anim.SetConstantPosition(nodes.Get(2),15,34);
   anim.SetConstantPosition(nodes.Get(3),15,44);
   anim.SetConstantPosition(nodes.Get(4),60,14);
   anim.SetConstantPosition(nodes.Get(5),60,24);
   anim.SetConstantPosition(nodes.Get(6),60,44);
   anim.SetConstantPosition(nodes.Get(7),60,54);
   anim.SetConstantPosition(nodes.Get(8),65,60);
   anim.SetConstantPosition(nodes.Get(9),34,34);
   anim.SetConstantPosition(nodes.Get(10),50,34);


//tracing
   bool tracing=true;  
   bool pcap = true;
   bool flow_monitor=true;
   std::string prefix_file_name = "TcpHybla";
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
      p2pbottle.EnablePcapAll (prefix_file_name, true);
      p2pdelay.EnablePcapAll (prefix_file_name, true);
    }

  // Flow monitor
  FlowMonitorHelper flowHelper;
  if (flow_monitor)
    {
      flowHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (100));
  Simulator::Run ();

  if (flow_monitor)
    {
      flowHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", true, true);
    }

   Simulator::Destroy ();
   return 0;
}
