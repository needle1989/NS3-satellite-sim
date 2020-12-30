#include <string>
#include <fstream>
#include <cstdlib>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"

typedef uint32_t uint;

using namespace ns3;

#define ERROR 0.000001

NS_LOG_COMPONENT_DEFINE ("CS349 Assignment-4 Group-43");

/* The definition of class APP and all its associated functions has been copied 
from files provided in examples/tutorials on download of ns3 itself*/
class APP: public Application {
	private:
		virtual void StartApplication(void);
		virtual void StopApplication(void);

		void ScheduleTx(void);
		void SendPacket(void);

		Ptr<Socket>     mSocket;
		Address         mPeer;
		uint32_t        mPacketSize;
		uint32_t        mNPackets;
		DataRate        mDataRate;
		EventId         mSendEvent;
		bool            mRunning;
		uint32_t        mPacketsSent;

	public:
		APP();
		virtual ~APP();

		void Setup(Ptr<Socket> socket, Address address, uint packetSize, uint nPackets, DataRate dataRate);
		void ChangeRate(DataRate newRate);
		void recv(int numBytesRcvd);

};

APP::APP(): mSocket(0),
		    mPeer(),
		    mPacketSize(0),
		    mNPackets(0),
		    mDataRate(0),
		    mSendEvent(),
		    mRunning(false),
		    mPacketsSent(0) {
}

APP::~APP() {
	mSocket = 0;
}

void APP::Setup(Ptr<Socket> socket, Address address, uint packetSize, uint nPackets, DataRate dataRate) {
	mSocket = socket;
	mPeer = address;
	mPacketSize = packetSize;
	mNPackets = nPackets;
	mDataRate = dataRate;
}

void APP::StartApplication() {
	mRunning = true;
	mPacketsSent = 0;
	mSocket->Bind();
	mSocket->Connect(mPeer);
	SendPacket();
}

void APP::StopApplication() {
	mRunning = false;
	if(mSendEvent.IsRunning()) {
		Simulator::Cancel(mSendEvent);
	}
	if(mSocket) {
		mSocket->Close();
	}
}

void APP::SendPacket() {
	Ptr<Packet> packet = Create<Packet>(mPacketSize);
	mSocket->Send(packet);

	if(++mPacketsSent < mNPackets) {
		ScheduleTx();
	}
}

void APP::ScheduleTx() {
	if (mRunning) {
		Time tNext(Seconds(mPacketSize*8/static_cast<double>(mDataRate.GetBitRate())));
		mSendEvent = Simulator::Schedule(tNext, &APP::SendPacket, this);
		//double tVal = Simulator::Now().GetSeconds();
		//if(tVal-int(tVal) >= 0.99)
		//	std::cout << Simulator::Now ().GetSeconds () << "\t" << mPacketsSent << std::endl;
	}
}

void APP::ChangeRate(DataRate newrate) {
	mDataRate = newrate;
	return;
}

//copied functions end here

//Helper function to configure a P2P connection (we require between hosts and routers and also between routers)
PointToPointHelper help_configP2P(std::string bandwidth, std::string delay, std::string queuesize)
{
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue(bandwidth)); //set bandwidth
	p2p.SetChannelAttribute("Delay", StringValue(delay));		//set delay
	//std::cout<<"Queue Size: "<<s<<std::endl;
	p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(queuesize));	//set queue size
	return p2p;
}

//Stores time vs congestion window size in file
static void Store_cwnd_size(Ptr<OutputStreamWrapper> stream, double startTime, uint oldCwnd, uint newCwnd) {
	//std::cout<<"Function Store_cwnd_size running \n";

	// storing time elapsed till now and current congestion window size into given filestream
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << newCwnd << std::endl; 
}

std::map<uint, uint> dropped_packets; //map to store flow number vs number of dropped packets due to buffer overflow

static void packet_dropped(Ptr<OutputStreamWrapper> stream, double startTime, uint flow_id) {
	//std::cout<<"Function packet_dropped running \n";

	//if a packet is getting dropped due to buffer overflow, we store the time of drop in the file
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << std::endl; 
	if(dropped_packets.find(flow_id) == dropped_packets.end()) {  //this flow got a packet dropped for the first time
		dropped_packets[flow_id] = 0;							  // hence initialising to zero
	}
	dropped_packets[flow_id]++; //incrementing the number of dropped packets
}

std::map<Address, double> total_bytes_gput;			  //stores total number of bytes received for each flow for goodput
std::map<std::string, double> total_bytes_tput;		  //stores total number of bytes received for each flow for throughput
std::map<std::string, double> max_throughput;		  //stores max throughput for each flow

// Function to calculate goodput and store the data into file
void Calc_gput(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, const Address& addr){
	//std::cout<<"Function Calc_gput running \n";

	double current_time = Simulator::Now().GetSeconds();
	if(total_bytes_gput.find(addr) == total_bytes_gput.end()) //if first packet received by flow
		total_bytes_gput[addr] = 0;

	total_bytes_gput[addr] += p->GetSize();					  //adding current packet size
	double gput_kbps = (((total_bytes_gput[addr] * 8.0) / 1024)/(current_time-startTime)); //calculating goodput
	*stream->GetStream() << current_time-startTime << "\t" <<  gput_kbps << std::endl;	   //storing in file
}

// Function to calculate throughput and store the data into file
void Calc_tput(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint interface) {
	//std::cout<<"Function Calc_tput running \n";

	double current_time = Simulator::Now().GetSeconds();

	//if first packet received by flow
	if(total_bytes_tput.find(context) == total_bytes_tput.end())
		total_bytes_tput[context] = 0;
	if(max_throughput.find(context) == max_throughput.end())
		max_throughput[context] = 0;

	total_bytes_tput[context] += p->GetSize();			//adding current packet size
	double tput_kbps = (((total_bytes_tput[context] * 8.0) / 1024)/(current_time-startTime)); //calculating throughput
	*stream->GetStream() << current_time-startTime << "\t" <<  tput_kbps << std::endl;	      //storing in file
	if(max_throughput[context] < tput_kbps)				//updating max throughput if required
		max_throughput[context] = tput_kbps;
}

// A function that accepts all the required values, assigns them to corresponding attributes and returns an ns3 TCP socket
Ptr<Socket> Create_Socket(Address sinkAddress, 
					uint sinkPort, 
					std::string tcpVariant, 
					Ptr<Node> hostNode, 
					Ptr<Node> sinkNode, 
					double startTime, 
					double stopTime,
					uint packetSize,
					uint numPackets,
					std::string dataRate,
					double appStartTime,
					double appStopTime) {

	//assigning variant type
	if(tcpVariant.compare("TcpHybla") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
	} else if(tcpVariant.compare("TcpWestwood+") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
	} else if(tcpVariant.compare("TcpYeah") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpYeah::GetTypeId()));
	} else {
		fprintf(stderr, "Invalid TCP version\n");
		exit(EXIT_FAILURE);
	}
	PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
	ApplicationContainer sinkApps = packetSinkHelper.Install(sinkNode);
	sinkApps.Start(Seconds(startTime));
	sinkApps.Stop(Seconds(stopTime));

	Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(hostNode, TcpSocketFactory::GetTypeId());
	
	//creating the APP
	Ptr<APP> app = CreateObject<APP>();
	app->Setup(ns3TcpSocket, sinkAddress, packetSize, numPackets, DataRate(dataRate));
	hostNode->AddApplication(app);
	app->SetStartTime(Seconds(appStartTime));
	app->SetStopTime(Seconds(appStopTime));

	return ns3TcpSocket;
}