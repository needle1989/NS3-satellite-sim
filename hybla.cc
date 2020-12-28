 #include <iostream>
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/netanim-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/point-to-point-layout-module.h"
 #include "ns3/mobility-module.h"
 
 using namespace ns3;
 
 class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}


void NetHelper (uint16_t sinkPort,NodeContainer nodes,Ipv4InterfaceContainer ixix,int num,int receiver,int sender,std::string rate,int cwnd)
{
   Address sinkAddress (InetSocketAddress (ixix.GetAddress (num), sinkPort));
   PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
   ApplicationContainer sinkApps= packetSinkHelper.Install (nodes.Get (receiver));
   sinkApps.Start (Seconds (0.));
   sinkApps.Stop (Seconds (20.));
   Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (sender), TcpSocketFactory::GetTypeId ());
   Ptr<MyApp> app = CreateObject<MyApp> ();
   app->Setup (ns3TcpSocket, sinkAddress, 1400, cwnd, DataRate (rate));
   nodes.Get (sender)->AddApplication (app);
   app->SetStartTime (Seconds (1.));
   app->SetStopTime (Seconds (20.));
}

 
 int main (int argc, char *argv[])
 {
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
   //Ipv4AddressHelper routerIP = Ipv4AddressHelper("15.4.0.0", "255.255.255.0");	//(network IP, mask)
   //Ipv4AddressHelper leftIP = Ipv4AddressHelper("15.1.0.0", "255.255.255.0");
   //Ipv4AddressHelper rightIP = Ipv4AddressHelper("15.2.0.0", "255.255.255.0");
   //Ipv4AddressHelper stalliteIP = Ipv4AddressHelper("15.3.0.0", "255.255.255.0");
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
   /*
    OnOffHelper onoff ("ns3::TcpSocketFactory",
                     Address (InetSocketAddress (ih7h8.GetAddress (1), 8080)));
  onoff.SetConstantRate (DataRate ("1Mbps"));
  ApplicationContainer apps = onoff.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));
  
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), 8080)));
  apps = sink.Install (nodes.Get (8));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));
   */
   
   /*uint16_t sinkPortRouter = 8080;
   Address sinkAddressRouter (InetSocketAddress (ir0r1.GetAddress (1), sinkPortRouter));
   PacketSinkHelper packetSinkHelperRouter ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPortRouter));
   ApplicationContainer sinkAppsRouter = packetSinkHelperRouter.Install (nodes.Get (10));
   sinkAppsRouter.Start (Seconds (0.));
   sinkAppsRouter.Stop (Seconds (20.));
   Ptr<Socket> ns3TcpSocketRouter = Socket::CreateSocket (nodes.Get (9), TcpSocketFactory::GetTypeId ());
   Ptr<MyApp> appRouter = CreateObject<MyApp> ();
   appRouter->Setup (ns3TcpSocketRouter, sinkAddressRouter, 1400, 1000, DataRate ("1Mbps"));
   nodes.Get (9)->AddApplication (appRouter);
   appRouter->SetStartTime (Seconds (1.));
   appRouter->SetStopTime (Seconds (20.));*/
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
    
   NetHelper (8080,nodes,ih7h8,1,8,0,"1Mbps",1000);   
   NetHelper (8081,nodes,ih6r1,0,6,1,"1Mbps",1000);  
   NetHelper (8082,nodes,ih5r1,0,5,2,"1Mbps",1000);  
   NetHelper (8083,nodes,ih4r1,0,4,3,"1Mbps",1000);  
   //NetHelper (8082,nodes,ih1r0,9,1,"1Mbps",1000);
   //NetHelper (8083,nodes,ih2r0,9,2,"1Mbps",1000);
   //NetHelper (8084,nodes,ih3r0,9,3,"1Mbps",1000);
   //NetHelper (8085,nodes,ih4r1,10,4,"1Mbps",1000);
   //NetHelper (8086,nodes,ih5r1,10,5,"1Mbps",1000);      
   //NetHelper (8087,nodes,ih6r1,10,6,"1Mbps",1000);     
   //NetHelper (8088,nodes,ih7r1,10,7,"1Mbps",1000);   
   //NetHelper (8089,nodes,ih7h8,8,7,"1Mbps",1000);  
      
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
   anim.SetConstantPosition(nodes.Get(4),70,14);
   anim.SetConstantPosition(nodes.Get(5),70,24);
   anim.SetConstantPosition(nodes.Get(6),70,44);
   anim.SetConstantPosition(nodes.Get(7),70,54);
   anim.SetConstantPosition(nodes.Get(8),75,60);
   anim.SetConstantPosition(nodes.Get(9),34,34);
   anim.SetConstantPosition(nodes.Get(10),60,34);

  
   Simulator::Run();
   Simulator::Destroy ();
   return 0;
}
