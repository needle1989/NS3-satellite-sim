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


//application class
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




//NetHelper

void NetHelper (uint16_t sinkPort,NodeContainer nodes,Ipv4InterfaceContainer ixix,int num,int receiver,int sender,std::string rate,int cwnd)
{
   Address sinkAddress (InetSocketAddress (ixix.GetAddress (num), sinkPort));
   PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
   ApplicationContainer sinkApps= packetSinkHelper.Install (nodes.Get (receiver));
   sinkApps.Start (Seconds (0.));
   sinkApps.Stop (Seconds (500.));
   Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (sender), TcpSocketFactory::GetTypeId ());
   Ptr<MyApp> app = CreateObject<MyApp> ();
   app->Setup (ns3TcpSocket, sinkAddress, 130000, cwnd, DataRate (rate));
   nodes.Get (sender)->AddApplication (app);
   app->SetStartTime (Seconds (1.));
   app->SetStopTime (Seconds (500.));
}

//Tracer
static bool firstCwnd = true;
static bool firstSshThr = true;
static bool firstRtt = true;
static bool firstRto = true;
static Ptr<OutputStreamWrapper> cWndStream;
static Ptr<OutputStreamWrapper> ssThreshStream;
static Ptr<OutputStreamWrapper> rttStream;
static Ptr<OutputStreamWrapper> rtoStream;
static Ptr<OutputStreamWrapper> nextTxStream;
static Ptr<OutputStreamWrapper> nextRxStream;
static Ptr<OutputStreamWrapper> inFlightStream;
static uint32_t cWndValue;
static uint32_t ssThreshValue;


static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue = newval;

  if (!firstSshThr)
    {
      *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << ssThreshValue << std::endl;
    }
}

static void
SsThreshTracer (uint32_t oldval, uint32_t newval)
{
  if (firstSshThr)
    {
      *ssThreshStream->GetStream () << "0.0 " << oldval << std::endl;
      firstSshThr = false;
    }
  *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  ssThreshValue = newval;

  if (!firstCwnd)
    {
      *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << cWndValue << std::endl;
    }
}

static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (Time oldval, Time newval)
{
  if (firstRto)
    {
      *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRto = false;
    }
  *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
NextTxTracer (SequenceNumber32 old, SequenceNumber32 nextTx)
{
  NS_UNUSED (old);
  *nextTxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
}

static void
InFlightTracer (uint32_t old, uint32_t inFlight)
{
  NS_UNUSED (old);
  *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
}

static void
NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
{
  NS_UNUSED (old);
  *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
}

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
}


static void
TraceSsThresh (std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  ssThreshStream = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold", MakeCallback (&SsThreshTracer));
}

static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
}

static void
TraceNextTx (std::string &next_tx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextTxStream = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence", MakeCallback (&NextTxTracer));
}

static void
TraceInFlight (std::string &in_flight_file_name)
{
  AsciiTraceHelper ascii;
  inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight", MakeCallback (&InFlightTracer));
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
   anim.SetConstantPosition(nodes.Get(4),70,14);
   anim.SetConstantPosition(nodes.Get(5),70,24);
   anim.SetConstantPosition(nodes.Get(6),70,44);
   anim.SetConstantPosition(nodes.Get(7),70,54);
   anim.SetConstantPosition(nodes.Get(8),75,60);
   anim.SetConstantPosition(nodes.Get(9),34,34);
   anim.SetConstantPosition(nodes.Get(10),60,34);

//tracing
   bool tracing=true;  
   bool pcap = true;
   bool flow_monitor=true;
   std::string prefix_file_name = "TcpVariantsComparison";
     if (tracing)
    {
      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
                                            std::ios::out);
      internet.EnableAsciiIpv4All (ascii_wrap);

      Simulator::Schedule (Seconds (0.1), &TraceCwnd, prefix_file_name + "-cwnd.data");
      Simulator::Schedule (Seconds (0.1), &TraceSsThresh, prefix_file_name + "-ssth.data");
      Simulator::Schedule (Seconds (0.1), &TraceRtt, prefix_file_name + "-rtt.data");
      Simulator::Schedule (Seconds (0.1), &TraceRto, prefix_file_name + "-rto.data");
      Simulator::Schedule (Seconds (0.1), &TraceNextTx, prefix_file_name + "-next-tx.data");
      Simulator::Schedule (Seconds (0.1), &TraceInFlight, prefix_file_name + "-inflight.data");
      Simulator::Schedule (Seconds (0.1), &TraceNextRx, prefix_file_name + "-next-rx.data");
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

  Simulator::Stop (Seconds (500));
  Simulator::Run ();

  if (flow_monitor)
    {
      flowHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", true, true);
    }

   Simulator::Destroy ();
   return 0;
}
