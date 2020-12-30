
/*
Details of the application:
Analyses and compares TCP Hybla, TCP Westwood+, and TCP YeAH-TCP performance. 
Selecting a Dumbbell topology with two routers R1 and R2 connected by a (10 Mbps, 50 ms) wired link. Each of the routers is
connected to 3 hosts, i.e. H1, H2, H3 (i.e. senders) are connected to R1, and H4, H5, H6 (i.e. receivers) are
connected to R2. The hosts are attached with (100 Mbps, 20 ms) links. Both the routers use drop-tail queues
with queue size set according to bandwidth-delay product. Senders (i.e. H1, H2 and H3) are attached with TCP
Hybla, TCP Westwood+, and TCP YeAH-TCP agents, respectively. 

Implementation detail:
		 _								_
		|	H1------+		+------H4	 |
		|			|		|			 |
Senders	|	H2------R1------R2-----H5	 |	Receivers
		|			|		|			 |
		|_	H3------+		+------H6	_|
	Representation in code:
	H1(n0), H2(n1), H3(n2), H4(n3), H5(n4), H6(n5), R1(n6), R2(n7) :: n stands for node
	Dumbbell topology is used with 
	Senders (left side of dumbbell) : H1, H2, H3  
	Receivers (right side of dumbbell) : H4, H5, H6
	and Routers R1 and R2 form the bridge of dumbbell.
	H1--H4 -> TCP Hybla
	H2--H5 -> TCP Westwood+
	H3--H6 -> TCP Yeah

	Details about the links:
	Hi---Rj= P2P (100 Mbps, 20ms) i=1,2...,6 j=1,2
	R1---R2= P2P (10 Mbps, 50ms)
	Bandwidth Delay Product (BDP)= number of bits that can fill up a network link = Bandwidth * Delay
	BDP for HiRj = 100 Mbps * 20 ms = 2000000 bits
	BDP for R1R2 = 10 Mbps * 50 ms = 500000 bits

	Packet size= 1.5 KB
*/

#include "header.h"
#include "ns3/netanim-module.h"

#define num_sender 3
#define Num_Packets 10000000

char types[num_sender][40]={"TcpHybla","TcpWestwood+","TcpYeah"};


//Function to initialize and configure the complete network of nodes
void Initialize(Ipv4InterfaceContainer* Receiver_Interfaces1,NodeContainer* Senders1,NodeContainer* Receivers1,int* Packet_Size1){ 
				//all variables passed by reference to reflect back changes
	
	//Given data to be used
	std::string HR_Bandwidth = "100Mbps";
	std::string HR_Delay = "20ms";
	std::string RR_Bandwidth = "10Mbps";
	std::string RR_Delay = "50ms";

	uint Packet_Size = 1.5*1024;		//Given packet size = 1.5KB = 1.5*1024 bits
	uint HR_QueueSize = (2000000)/Packet_Size;  //Number of packets = Queue size= Bandwidth Delay Product / Packet size
	uint RR_QueueSize = (500000)/Packet_Size;
	std::cout<<"Queue Size for Host to Router links: "<<(std::to_string(HR_QueueSize)+" Packets")<<std::endl;
	std::cout<<"Queue Size for Router to Router link: "<<(std::to_string(RR_QueueSize)+" Packets")<<std::endl<<std::endl;
	
	double error = ERROR;

	//Configuring the network
	PointToPointHelper p2pHR = help_configP2P(HR_Bandwidth, HR_Delay, std::to_string(HR_QueueSize)+"p" );
	PointToPointHelper p2pRR = help_configP2P(RR_Bandwidth, RR_Delay, std::to_string(RR_QueueSize)+"p" );
	Ptr<RateErrorModel> Error_Model = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate", DoubleValue (error));

	//Initialising node containers
	std::cout << "Initialising node containers..."<< std::endl;
	NodeContainer Routers, Senders, Receivers;

	//Create n nodes and append pointers to them to the end of this NodeContainer. 
	Routers.Create(2);
	Senders.Create(num_sender);
	Receivers.Create(num_sender);

	//Initialising NetDeviceContatiners
	NetDeviceContainer routerDevices = p2pRR.Install(Routers);
	
	NetDeviceContainer leftRouterDevices, rightRouterDevices, senderDevices, ReceiverDevices;

	//Adding links
	std::cout << "Adding and configuring links between nodes..."<< std::endl;

	for(int i=0;i<num_sender;i++)
	{
		//Configuring all the HiR1 links i=1,2,3 (senders to R1 links)
		NetDeviceContainer Config_Left = p2pHR.Install(Routers.Get(0), Senders.Get(i)); 
		leftRouterDevices.Add(Config_Left.Get(0)); // left router
		senderDevices.Add(Config_Left.Get(1)); // all devices on left of dumbbell
		Config_Left.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(Error_Model));
	}

	for(int i=0;i<num_sender;i++)
	{
		//Configuring all the HiR2 links i=4,5,6 (receivers to R2 links)
		NetDeviceContainer Config_Right = p2pHR.Install(Routers.Get(1), Receivers.Get(i)); 
		rightRouterDevices.Add(Config_Right.Get(0)); // right router
		ReceiverDevices.Add(Config_Right.Get(1)); // all devices on right of dumbbell
		Config_Right.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(Error_Model));
	}

	
	//Installing Internet Stack
	std::cout << "Installing internet stack on the nodes..."<< std::endl;
	InternetStackHelper stack;
	stack.Install(Routers);
	stack.Install(Senders);
	stack.Install(Receivers);

	//Assigning IP addresses to the nodes
	std::cout << "Assigning IP addresses to the nodes and initialising network interfaces..."<< std::endl;
	Ipv4AddressHelper routerIP = Ipv4AddressHelper("15.3.0.0", "255.255.255.0");	//(network IP, mask)
	Ipv4AddressHelper senderIP = Ipv4AddressHelper("15.1.0.0", "255.255.255.0");
	Ipv4AddressHelper ReceiverIP = Ipv4AddressHelper("15.2.0.0", "255.255.255.0");

	Ipv4InterfaceContainer router_Interface, sender_Interfaces, Receiver_Interfaces, leftRouter_Interfaces, rightRouter_Interfaces;

	router_Interface = routerIP.Assign(routerDevices);

	for(int i=0;i<num_sender;i++) 
	{
		NetDeviceContainer senderDevice;
		senderDevice.Add(senderDevices.Get(i));
		senderDevice.Add(leftRouterDevices.Get(i));

		Ipv4InterfaceContainer sender_Interface = senderIP.Assign(senderDevice);
		sender_Interfaces.Add(sender_Interface.Get(0));
		leftRouter_Interfaces.Add(sender_Interface.Get(1));
		senderIP.NewNetwork();

		NetDeviceContainer ReceiverDevice;
		ReceiverDevice.Add(ReceiverDevices.Get(i));
		ReceiverDevice.Add(rightRouterDevices.Get(i));
		
		Ipv4InterfaceContainer Receiver_Interface = ReceiverIP.Assign(ReceiverDevice);
		Receiver_Interfaces.Add(Receiver_Interface.Get(0));
		rightRouter_Interfaces.Add(Receiver_Interface.Get(1));
		ReceiverIP.NewNetwork();
	}

	std::cout<<"Initialisation of network finished"<< std::endl<<std::endl;

	// Returning the values back after initialisation
	*Receiver_Interfaces1=Receiver_Interfaces;
	*Senders1=Senders;
	*Receivers1=Receivers;
	*Packet_Size1=Packet_Size;

}

//Function to create files to store all of the data 
void Create_files(int i,Ptr<OutputStreamWrapper> * tput,
				  Ptr<OutputStreamWrapper> * cwnd,
				  Ptr<OutputStreamWrapper> * closs,
				  Ptr<OutputStreamWrapper> * gput,
				  int type)
{	AsciiTraceHelper asciiTraceHelper;
	std:: string filename=types[i];
	if(type==1)
		filename="data_"+filename+"_a";
	else
		filename="data_"+filename+"_b";
	cwnd[i] = asciiTraceHelper.CreateFileStream(filename+".cw");  //file to store congestion window data for i-th network type
	closs[i] = asciiTraceHelper.CreateFileStream(filename+".cl"); //file to store congestion loss data for i-th network type
	tput[i] = asciiTraceHelper.CreateFileStream(filename+".tp");  //file to store throughput data for i-th network type
	gput[i] = asciiTraceHelper.CreateFileStream(filename+".gp");  //file to store goodput data for i-th network type
}

//Function to store statistics into file
void StoreStatsInFile(std::map<FlowId, FlowMonitor::FlowStats> stats,
					  Ptr<OutputStreamWrapper> *closs,
					  Ptr<Ipv4FlowClassifier> classifier)
{		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		int type=-1;
		if(t.sourceAddress == "15.1.0.1") {
			type=0;
			*closs[type]->GetStream()<<"** TCP Hybla Flow **\n"<<"Flow ID :"<<i->first<<"\n";	
			*closs[type]->GetStream()<<"Maximum throughput: "<<max_throughput["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"]<<std::endl;
		} 
		else if(t.sourceAddress == "15.1.1.1") {
			type=1;
			*closs[type]->GetStream()<<"** TCP Westwood+ Flow **\n"<<"Flow ID :"<<i->first<<"\n";
			*closs[type]->GetStream()<<"Maximum throughput: "<<max_throughput["/NodeList/6/$ns3::Ipv4L3Protocol/Rx"]<<std::endl;
		} 
		else if(t.sourceAddress == "15.1.2.1") {
			type=2;
			*closs[type]->GetStream()<<"** TCP YeAH-TCP Flow **\n"<<"Flow ID :"<<i->first<<"\n";
			*closs[type]->GetStream()<<"Maximum throughput: "<<max_throughput["/NodeList/7/$ns3::Ipv4L3Protocol/Rx"]<<std::endl;
		}
		//std::cout<<type<<"  "<<t.sourceAddress<<std::endl;
		if(type>=0){
		if(dropped_packets.find(type+1)==dropped_packets.end())
		 	dropped_packets[type+1] = 0;
		*closs[type]->GetStream()<<"Source IP: "<< t.sourceAddress << " -> Destination IP: "<<t.destinationAddress<<std::endl;
		*closs[type]->GetStream()<<"Total number of packets transmitted: "<<Num_Packets<<std::endl;
		*closs[type]->GetStream()<<"Total number of packets lost: "<<i->second.lostPackets<<std::endl;
		*closs[type]->GetStream()<<"Number of packets transferred successfully: "<<Num_Packets-(i->second.lostPackets)<<std::endl;
		*closs[type]->GetStream()<<"Number of packets lost due to buffer overflow: "<< dropped_packets[type+1]<<std::endl;
		*closs[type]->GetStream()<<"Number of packets lost due to congestion: "<<i->second.lostPackets - dropped_packets[type+1]<<std::endl;
		*closs[type]->GetStream()<<"% loss due to buffer overflow: "<<(dropped_packets[type+1]*100.0)/Num_Packets<<std::endl;
		*closs[type]->GetStream()<<"% loss due to congestion: "<<((i->second.lostPackets - dropped_packets[type+1])*100.0)/Num_Packets<<std::endl;

	}
	}
}

/* Function to execute part 1 and 3:
Start only one flow and analyse the throughput over sufficiently long duration.
Plot the evolution of congestion window w.r.t. time. Perform this experiment with all the 
flows attached to all the three sending agents. 
Measure the congestion loss and the goodput over the duration of the experiment for each of the flows.*/

void Part13() 
{
	
	std::cout << "*** Executing part 1 and 3 ***" << std::endl;

	Ipv4InterfaceContainer Receiver_Interfaces;
	NodeContainer Senders, Receivers;
	int Packet_Size;

	//Initialising the network
	Initialize(&Receiver_Interfaces,&Senders,&Receivers,&Packet_Size);

	//Initializing required variables
	double Duration_Gap = 100;
	double Net_Duration = 0;
	uint port = 2500;
	std::string Transfer_Speed  = "400Mbps";	

	// arrays to store output streams for Throughput, Congestion Window, Congestion Loss, Goodput data respectively for each TCP variant
	Ptr<OutputStreamWrapper> *tput=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *cwnd=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *closs=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *gput=new Ptr<OutputStreamWrapper>[num_sender];

	// array to store ns3 TCP sockets for each TCP variant
	Ptr<Socket>* ns3TCPSocket = new Ptr<Socket>[num_sender];

	//array to store sink values for Goodput
	std:: string * sink1= new std::string[num_sender];
	sink1[0]="/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[1]="/NodeList/6/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[2]="/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx";

	//array to store sink values for Throughput
	std:: string * sink2= new std::string[num_sender];
	sink2[0]="/NodeList/5/$ns3::Ipv4L3Protocol/Rx";
	sink2[1]="/NodeList/6/$ns3::Ipv4L3Protocol/Rx";
	sink2[2]="/NodeList/7/$ns3::Ipv4L3Protocol/Rx";
	

	//Measuring Performance of each TCP variant
	std::cout << "Measuring Performance of each TCP variant one by one..." << std::endl;

	for(int i=0;i<num_sender;i++){
		std::cout<<"From H"<<i+1<<" to H"<<i+4<<" ::: ";;
		std::cout<<"Connection type: "<<types[i]<<std::endl;
		Create_files(i,tput,cwnd,closs,gput,1);   //create files to store data

		// assigning attributes to ns3 TCP sockets
		ns3TCPSocket[i] = Create_Socket(InetSocketAddress(Receiver_Interfaces.GetAddress(i), port), port,types[i], Senders.Get(i), Receivers.Get(i), Net_Duration, Net_Duration+Duration_Gap, Packet_Size, Num_Packets, Transfer_Speed , Net_Duration, Net_Duration+Duration_Gap);
		ns3TCPSocket[i]->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&Store_cwnd_size, cwnd[i], Net_Duration));
		ns3TCPSocket[i]->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packet_dropped, closs[i], Net_Duration, i+1));
		
		// Measuring packet sinks
		Config::Connect(sink1[i], MakeBoundCallback(&Calc_gput, gput[i], Net_Duration));
		Config::Connect(sink2[i], MakeBoundCallback(&Calc_tput, tput[i], Net_Duration));

		Net_Duration += Duration_Gap; //update net duration elapsed
	}

	
	std::cout<<"Measuring performances finished"<<std::endl<<std::endl;

	//Building a routing database and initializing the routing tables of the nodes in the simulation
	std::cout<<"Populating Routing Tables..."<< std::endl;
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	// Enabling IP flow monitoring on all the nodes 
	std::cout<<"Setting up FlowMonitor to enable IP flow monitoring on all the nodes..."<<std::endl;
	Ptr<FlowMonitor> flowmon;
	FlowMonitorHelper flowmonHelper;
	flowmon = flowmonHelper.InstallAll();
	Simulator::Stop(Seconds(Net_Duration));

	// Simulation starts
	std::cout<<"Starting Simulation!"<< std::endl;
        AnimationInterface anim ("test1.xml");
	Simulator::Run();
	
	std::cout<<"Checking for lost packets..."<< std::endl;
	flowmon->CheckForLostPackets();

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

	// Retrieving all collected the flow statistics
	std::cout<<"Collecting flow statistics..."<< std::endl;
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
	StoreStatsInFile(stats,closs,classifier); //storing flow statistics in file

	Simulator::Destroy();
	std::cout << "Simulation finished! Data has been stored. " << std::endl; 
	// Data is stored in folder ns-3.30.1 where 3.30.1 is the version of ns3 user is using
	return;
}
 
/* Function to execute part 2 and 3:
Start 2 other flows sharing the bottleneck link while the first one is in progress
and measure the throughput (in Kbps) of each flow. Plot the throughput and evolution of the TCP
congestion window for each of the flow at a steady-state. Report the maximum throughput observed for
each of the flows.
Measure the congestion loss and the goodput over the duration of the experiment for each of the flows. */

void Part23()
{
	std::cout << "*** Executing part 2 and 3 ***" << std::endl;
	
	Ipv4InterfaceContainer Receiver_Interfaces;
	NodeContainer Senders, Receivers;
	int Packet_Size;

	//Initialising the network
	Initialize(&Receiver_Interfaces,&Senders,&Receivers,&Packet_Size);

	//Initializing required variables
	double Duration_Gap = 100;
	double FirstFlowStart = 0;  //first flow starts at time 0
	double OtherFlowStart = 20; //time when other flows start, while first one in progress
	uint port = 9000;
	std::string Transfer_Speed  = "400Mbps";
		
	// arrays to store output streams for Throughput, Congestion Window, Congestion Loss, Goodput data respectively for each TCP variant
	Ptr<OutputStreamWrapper> *tput=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *cwnd=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *closs=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *gput=new Ptr<OutputStreamWrapper>[num_sender];

	// array to store ns3 TCP sockets for each TCP variant
	Ptr<Socket>* ns3TCPSocket = new Ptr<Socket>[num_sender];

	//array to store sink values for Goodput
	std:: string * sink1= new std::string[num_sender];
	sink1[0]="/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[1]="/NodeList/6/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[2]="/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx";
	
	//array to store sink values for Throughput
	std:: string * sink2= new std::string[num_sender];
	sink2[0]="/NodeList/5/$ns3::Ipv4L3Protocol/Rx";
	sink2[1]="/NodeList/6/$ns3::Ipv4L3Protocol/Rx";
	sink2[2]="/NodeList/7/$ns3::Ipv4L3Protocol/Rx";
	

	for(int i=0;i<num_sender;i++){
		std::cout<<"From H"<<i<<" to H"<<i+4<<" : ";
		std::cout<<"Connection type: "<<types[i]<<std::endl;
		Create_files(i,tput,cwnd,closs,gput,2); //create files to store data

		// assigning attributes to ns3 TCP sockets
		if(i==0) //First flow starts at FirstFlowStart=0
		ns3TCPSocket[i] = Create_Socket(InetSocketAddress(Receiver_Interfaces.GetAddress(i), port), port,types[i], Senders.Get(i), Receivers.Get(i), FirstFlowStart, FirstFlowStart+Duration_Gap, Packet_Size, Num_Packets, Transfer_Speed , FirstFlowStart, FirstFlowStart+Duration_Gap);
		else     //Other flows start at FirstFlowStart+OtherFlowStart = 20, while first one in progress
		ns3TCPSocket[i] = Create_Socket(InetSocketAddress(Receiver_Interfaces.GetAddress(i), port), port,types[i], Senders.Get(i), Receivers.Get(i), OtherFlowStart, OtherFlowStart+Duration_Gap, Packet_Size, Num_Packets, Transfer_Speed , OtherFlowStart, OtherFlowStart+Duration_Gap);
		
		ns3TCPSocket[i]->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&Store_cwnd_size, cwnd[i], 0));
		ns3TCPSocket[i]->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packet_dropped, closs[i], 0, i+1));
		
		// Measuring packet sinks
		Config::Connect(sink1[i], MakeBoundCallback(&Calc_gput, gput[i], 0));
		Config::Connect(sink2[i], MakeBoundCallback(&Calc_tput, tput[i], 0));

	}

	std::cout<<std::endl;

	//Building a routing database and initializing the routing tables of the nodes in the simulation
	std::cout<<"Populating Routing Tables..."<< std::endl;
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	// Enabling IP flow monitoring on all the nodes 
	std::cout<<"Setting up FlowMonitor to enable IP flow monitoring on all the nodes..."<<std::endl;
	Ptr<FlowMonitor> flowmon;
	FlowMonitorHelper flowmonHelper;
	flowmon = flowmonHelper.InstallAll();
	Simulator::Stop(Seconds(OtherFlowStart+Duration_Gap));

	// Simulation starts
	std::cout<<"Starting Simulation!"<< std::endl;
	Simulator::Run();
	
	std::cout<<"Checking for lost packets..."<< std::endl;
	flowmon->CheckForLostPackets();

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

	// Retrieving all collected the flow statistics
	std::cout<<"Collecting flow statistics..."<< std::endl;
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
	StoreStatsInFile(stats,closs,classifier);

	Simulator::Destroy();
	std::cout << "Simulation finished! Data has been stored. " << std::endl; 
	// Data is stored in folder ns-3.30.1 where 3.30.1 is the version of ns3 user is using
	return;
}

int main(){
	char ans='Y';
	int response;
	while((ans=='Y')||(ans=='y')){
		std::cout<<"Enter your choice: (1 or 2)\n";
		std::cout<<"1. Execute part 1 and 3 \n";
		std::cout<<"2. Execute part 2 and 3 \n";
		std::cin>>response;
		if(response==1)
			Part13();
		else Part23();
		std::cout<<"Do you want to perform another simulation? (Y/N) \n";
		std::cin>>ans;
	}
}
