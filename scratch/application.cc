/*
GPL License
----------------

Application 6:
Detail...



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
	H1, H2, H3 on left side of dumbbell,
	H4, H5, H6 on right side of dumbbell,
	and routers R1 and R2 form the bridge of dumbbell.

	H1 is attached with TCP Reno agent.
	H2 is attached with TCP Vegas agent.
	H3 is attached with TCP Fack agent.

	Links:
	H1R1/H2R1/H3R1/H4R2/H5R2/H6R2: P2P with 100Mbps and 20ms.
	R1R2: (dumbbell bridge) P2P with 10Mbps and 50ms.

	packet size: 1.2KB.
	Number of packets decided by Bandwidth delay product:
	i.e. #packets = Bandwidth*Delay(in bits)
	Therefore, max #packets (HiRj) = 100Mbps*20ms = 2000000
	and max #packets (R1R2) = 10Mbps*50ms = 500000

*/
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

typedef uint32_t uint;

using namespace ns3;

#define ERROR 0.000001


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

static void CwndChange(Ptr<OutputStreamWrapper> stream, double startTime, uint oldCwnd, uint newCwnd) {
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << newCwnd << std::endl;
}

std::map<uint, uint> mapDrop;
static void packetDrop(Ptr<OutputStreamWrapper> stream, double startTime, uint myId) {
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << std::endl;
	if(mapDrop.find(myId) == mapDrop.end()) {
		mapDrop[myId] = 0;
	}
	mapDrop[myId]++;
}


void IncRate(Ptr<APP> app, DataRate rate) {
	app->ChangeRate(rate);
	return;
}

std::map<Address, double> mapBytesReceived;
std::map<std::string, double> mapBytesReceivedIPV4, mapMaxThroughput;
static double lastTimePrint = 0, lastTimePrintIPV4 = 0;
double printGap = 0;

void ReceivedPacket(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, const Address& addr){
	double timeNow = Simulator::Now().GetSeconds();

	if(mapBytesReceived.find(addr) == mapBytesReceived.end())
		mapBytesReceived[addr] = 0;
	mapBytesReceived[addr] += p->GetSize();
	double kbps_ = (((mapBytesReceived[addr] * 8.0) / 1024)/(timeNow-startTime));
	if(timeNow - lastTimePrint >= printGap) {
		lastTimePrint = timeNow;
		*stream->GetStream() << timeNow-startTime << "\t" <<  kbps_ << std::endl;
	}
}

void ReceivedPacketIPV4(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint interface) {
	double timeNow = Simulator::Now().GetSeconds();

	if(mapBytesReceivedIPV4.find(context) == mapBytesReceivedIPV4.end())
		mapBytesReceivedIPV4[context] = 0;
	if(mapMaxThroughput.find(context) == mapMaxThroughput.end())
		mapMaxThroughput[context] = 0;
	mapBytesReceivedIPV4[context] += p->GetSize();
	double kbps_ = (((mapBytesReceivedIPV4[context] * 8.0) / 1024)/(timeNow-startTime));
	if(timeNow - lastTimePrintIPV4 >= printGap) {
		lastTimePrintIPV4 = timeNow;
		*stream->GetStream() << timeNow-startTime << "\t" <<  kbps_ << std::endl;
		if(mapMaxThroughput[context] < kbps_)
			mapMaxThroughput[context] = kbps_;
	}
}


Ptr<Socket> uniFlow(Address sinkAddress, 
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

	if(tcpVariant.compare("TcpReno") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpReno::GetTypeId()));
	} else if(tcpVariant.compare("TcpTahoe") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpTahoe::GetTypeId()));
	} else if(tcpVariant.compare("TcpWestwood") == 0) {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
	} else {
		fprintf(stderr, "Invalid TCP version\n");
		exit(EXIT_FAILURE);
	}
	PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
	ApplicationContainer sinkApps = packetSinkHelper.Install(sinkNode);
	sinkApps.Start(Seconds(startTime));
	sinkApps.Stop(Seconds(stopTime));

	Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(hostNode, TcpSocketFactory::GetTypeId());
	

	Ptr<APP> app = CreateObject<APP>();
	app->Setup(ns3TcpSocket, sinkAddress, packetSize, numPackets, DataRate(dataRate));
	hostNode->AddApplication(app);
	app->SetStartTime(Seconds(appStartTime));
	app->SetStopTime(Seconds(appStopTime));

	return ns3TcpSocket;
}

void partAC() {
	std::cout << "Part A started..." << std::endl;
	std::string rateHR = "100Mbps";
	std::string latencyHR = "20ms";
	std::string rateRR = "10Mbps";
	std::string latencyRR = "50ms";

	uint packetSize = 1.2*1024;		//1.2KB
	uint queueSizeHR = (100000*20)/packetSize;
	uint queueSizeRR = (10000*50)/packetSize;

	uint numSender = 3;

	double errorP = ERROR;


	Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
    /*
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(queuesize));
	*/

	//Creating channel without IP address
	PointToPointHelper p2pHR, p2pRR;
	p2pHR.SetDeviceAttribute("DataRate", StringValue(rateHR));
	p2pHR.SetChannelAttribute("Delay", StringValue(latencyHR));
	p2pHR.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queueSizeHR));
	p2pRR.SetDeviceAttribute("DataRate", StringValue(rateRR));
	p2pRR.SetChannelAttribute("Delay", StringValue(latencyRR));
	p2pRR.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queueSizeRR));

	//Adding some errorrate
	Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate", DoubleValue (errorP));

	NodeContainer routers, senders, receivers;
	routers.Create(2);
	senders.Create(numSender);
	receivers.Create(numSender);

	NetDeviceContainer routerDevices = p2pRR.Install(routers);
	NetDeviceContainer leftRouterDevices, rightRouterDevices, senderDevices, receiverDevices;

	//Adding links
	for(uint i = 0; i < numSender; ++i) {
		NetDeviceContainer cleft = p2pHR.Install(routers.Get(0), senders.Get(i));
		leftRouterDevices.Add(cleft.Get(0));
		senderDevices.Add(cleft.Get(1));
		cleft.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));

		NetDeviceContainer cright = p2pHR.Install(routers.Get(1), receivers.Get(i));
		rightRouterDevices.Add(cright.Get(0));
		receiverDevices.Add(cright.Get(1));
		cright.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
	}

	//Install Internet Stack
	InternetStackHelper stack;
	stack.Install(routers);
	stack.Install(senders);
	stack.Install(receivers);


	//Adding IP addresses
	Ipv4AddressHelper routerIP = Ipv4AddressHelper("10.3.0.0", "255.255.255.0");
	Ipv4AddressHelper senderIP = Ipv4AddressHelper("10.1.0.0", "255.255.255.0");
	Ipv4AddressHelper receiverIP = Ipv4AddressHelper("10.2.0.0", "255.255.255.0");
	

	Ipv4InterfaceContainer routerIFC, senderIFCs, receiverIFCs, leftRouterIFCs, rightRouterIFCs;

	routerIFC = routerIP.Assign(routerDevices);

	for(uint i = 0; i < numSender; ++i) {
		NetDeviceContainer senderDevice;
		senderDevice.Add(senderDevices.Get(i));
		senderDevice.Add(leftRouterDevices.Get(i));
		Ipv4InterfaceContainer senderIFC = senderIP.Assign(senderDevice);
		senderIFCs.Add(senderIFC.Get(0));
		leftRouterIFCs.Add(senderIFC.Get(1));
		senderIP.NewNetwork();

		NetDeviceContainer receiverDevice;
		receiverDevice.Add(receiverDevices.Get(i));
		receiverDevice.Add(rightRouterDevices.Get(i));
		Ipv4InterfaceContainer receiverIFC = receiverIP.Assign(receiverDevice);
		receiverIFCs.Add(receiverIFC.Get(0));
		rightRouterIFCs.Add(receiverIFC.Get(1));
		receiverIP.NewNetwork();
	}

	/*
		Measuring Performance of each TCP variant
	*/
	//To be written
	


	
	/********************************************************************
	PART (a)
	********************************************************************/
	/********************************************************************
		One flow for each tcp_variant and measure
		1) Throughput for long durations
		2) Evolution of Congestion window
	********************************************************************/
	double durationGap = 100;
	double netDuration = 0;
	uint port = 9000;
	uint numPackets = 10000000;
	std::string transferSpeed = "400Mbps";	

	//TCP Reno from H1 to H4
	AsciiTraceHelper asciiTraceHelper;
	Ptr<OutputStreamWrapper> stream1CWND = asciiTraceHelper.CreateFileStream("application_6_h1_h4_a.cwnd");
	Ptr<OutputStreamWrapper> stream1PD = asciiTraceHelper.CreateFileStream("application_6_h1_h4_a.congestion_loss");
	Ptr<OutputStreamWrapper> stream1TP = asciiTraceHelper.CreateFileStream("application_6_h1_h4_a.tp");
	Ptr<OutputStreamWrapper> stream1GP = asciiTraceHelper.CreateFileStream("application_6_h1_h4_a.gp");
	Ptr<Socket> ns3TcpSocket1 = uniFlow(InetSocketAddress(receiverIFCs.GetAddress(0), port), port, "TcpReno", senders.Get(0), receivers.Get(0), netDuration, netDuration+durationGap, packetSize, numPackets, transferSpeed, netDuration, netDuration+durationGap);
	ns3TcpSocket1->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndChange, stream1CWND, netDuration));
	ns3TcpSocket1->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream1PD, netDuration, 1));

	// Measure PacketSinks
	std::string sink = "/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&ReceivedPacket, stream1GP, netDuration));

	std::string sink_ = "/NodeList/5/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&ReceivedPacketIPV4, stream1TP, netDuration));

	netDuration += durationGap;


	//TCP Tahoe from H2 to H5
	Ptr<OutputStreamWrapper> stream2CWND = asciiTraceHelper.CreateFileStream("application_6_h2_h5_a.cwnd");
	Ptr<OutputStreamWrapper> stream2PD = asciiTraceHelper.CreateFileStream("application_6_h2_h5_a.congestion_loss");
	Ptr<OutputStreamWrapper> stream2TP = asciiTraceHelper.CreateFileStream("application_6_h2_h5_a.tp");
	Ptr<OutputStreamWrapper> stream2GP = asciiTraceHelper.CreateFileStream("application_6_h2_h5_a.gp");
	Ptr<Socket> ns3TcpSocket2 = uniFlow(InetSocketAddress(receiverIFCs.GetAddress(1), port), port, "TcpTahoe", senders.Get(1), receivers.Get(1), netDuration, netDuration+durationGap, packetSize, numPackets, transferSpeed, netDuration, netDuration+durationGap);
	ns3TcpSocket2->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndChange, stream2CWND, netDuration));
	ns3TcpSocket2->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream2PD, netDuration, 2));

	sink = "/NodeList/6/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&ReceivedPacket, stream2GP, netDuration));
	sink_ = "/NodeList/6/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&ReceivedPacketIPV4, stream2TP, netDuration));
	netDuration += durationGap;

	//TCP WestWood from H3 to H6
	Ptr<OutputStreamWrapper> stream3CWND = asciiTraceHelper.CreateFileStream("application_6_h3_h6_a.cwnd");
	Ptr<OutputStreamWrapper> stream3PD = asciiTraceHelper.CreateFileStream("application_6_h3_h6_a.congestion_loss");
	Ptr<OutputStreamWrapper> stream3TP = asciiTraceHelper.CreateFileStream("application_6_h3_h6_a.tp");
	Ptr<OutputStreamWrapper> stream3GP = asciiTraceHelper.CreateFileStream("application_6_h3_h6_a.gp");
	Ptr<Socket> ns3TcpSocket3 = uniFlow(InetSocketAddress(receiverIFCs.GetAddress(2), port), port, "TcpWestwood", senders.Get(2), receivers.Get(2), netDuration, netDuration+durationGap, packetSize, numPackets, transferSpeed, netDuration, netDuration+durationGap);
	ns3TcpSocket3->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndChange, stream3CWND, netDuration));
	ns3TcpSocket3->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream3PD, netDuration, 3));

	sink = "/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&ReceivedPacket, stream3GP, netDuration));
	sink_ = "/NodeList/7/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&ReceivedPacketIPV4, stream3TP, netDuration));
	netDuration += durationGap;

	//p2pHR.EnablePcapAll("application_6__a");
	//p2pRR.EnablePcapAll("application_6_RR_a");

	//Turning on Static Global Routing
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Ptr<FlowMonitor> flowmon;
	FlowMonitorHelper flowmonHelper;
	flowmon = flowmonHelper.InstallAll();
	Simulator::Stop(Seconds(netDuration));
	Simulator::Run();
	flowmon->CheckForLostPackets();

	//Ptr<OutputStreamWrapper> streamTP = asciiTraceHelper.CreateFileStream("application_6_a.tp");
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		/*
		*streamTP->GetStream()  << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
		*streamTP->GetStream()  << "  Tx Bytes:   " << i->second.txBytes << "\n";
		*streamTP->GetStream()  << "  Rx Bytes:   " << i->second.rxBytes << "\n";
		*streamTP->GetStream()  << "  Time        " << i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds() << "\n";
		*streamTP->GetStream()  << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";	
		*/
		if(t.sourceAddress == "10.1.0.1") {
			if(mapDrop.find(1)==mapDrop.end())
				mapDrop[1] = 0;
			*stream1PD->GetStream() << "TcpReno Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			*stream1PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream1PD->GetStream()  << "Packet Lost due to buffer overflow: " << mapDrop[1] << "\n";
			*stream1PD->GetStream()  << "Packet Lost due to Congestion: " << i->second.lostPackets - mapDrop[1] << "\n";
			*stream1PD->GetStream() << "Max throughput: " << mapMaxThroughput["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
		} else if(t.sourceAddress == "10.1.1.1") {
			if(mapDrop.find(2)==mapDrop.end())
				mapDrop[2] = 0;
			*stream2PD->GetStream() << "Tcp Tahoe Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			*stream2PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream2PD->GetStream()  << "Packet Lost due to buffer overflow: " << mapDrop[2] << "\n";
			*stream2PD->GetStream()  << "Packet Lost due to Congestion: " << i->second.lostPackets - mapDrop[2] << "\n";
			*stream2PD->GetStream() << "Max throughput: " << mapMaxThroughput["/NodeList/6/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
		} else if(t.sourceAddress == "10.1.2.1") {
			if(mapDrop.find(3)==mapDrop.end())
				mapDrop[3] = 0;
			*stream3PD->GetStream() << "Tcp WestWood Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			*stream3PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream3PD->GetStream()  << "Packet Lost due to buffer overflow: " << mapDrop[3] << "\n";
			*stream3PD->GetStream()  << "Packet Lost due to Congestion: " << i->second.lostPackets - mapDrop[3] << "\n";
			*stream3PD->GetStream() << "Max throughput: " << mapMaxThroughput["/NodeList/7/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
		}
	}

	//flowmon->SerializeToXmlFile("application_6_a.flowmon", true, true);
	Simulator::Destroy();
}


void partBC() {
	std::cout << "Part B started..." << std::endl;
	std::string rateHR = "100Mbps";
	std::string latencyHR = "20ms";
	std::string rateRR = "10Mbps";
	std::string latencyRR = "50ms";

	uint packetSize = 1.2*1024;		//1.2KB
	uint queueSizeHR = (100000*20)/packetSize;
	uint queueSizeRR = (10000*50)/packetSize;

	uint numSender = 3;

	double errorP = ERROR;


	Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
    /*
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(queuesize));
	*/

	//Creating channel without IP address
	PointToPointHelper p2pHR, p2pRR;
	p2pHR.SetDeviceAttribute("DataRate", StringValue(rateHR));
	p2pHR.SetChannelAttribute("Delay", StringValue(latencyHR));
	p2pHR.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queueSizeHR));
	p2pRR.SetDeviceAttribute("DataRate", StringValue(rateRR));
	p2pRR.SetChannelAttribute("Delay", StringValue(latencyRR));
	p2pRR.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queueSizeRR));

	//Adding some errorrate
	Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate", DoubleValue (errorP));

	NodeContainer routers, senders, receivers;
	routers.Create(2);
	senders.Create(numSender);
	receivers.Create(numSender);

	NetDeviceContainer routerDevices = p2pRR.Install(routers);
	NetDeviceContainer leftRouterDevices, rightRouterDevices, senderDevices, receiverDevices;

	//Adding links
	for(uint i = 0; i < numSender; ++i) {
		NetDeviceContainer cleft = p2pHR.Install(routers.Get(0), senders.Get(i));
		leftRouterDevices.Add(cleft.Get(0));
		senderDevices.Add(cleft.Get(1));
		cleft.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));

		NetDeviceContainer cright = p2pHR.Install(routers.Get(1), receivers.Get(i));
		rightRouterDevices.Add(cright.Get(0));
		receiverDevices.Add(cright.Get(1));
		cright.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
	}

	//Install Internet Stack
	InternetStackHelper stack;
	stack.Install(routers);
	stack.Install(senders);
	stack.Install(receivers);


	//Adding IP addresses
	Ipv4AddressHelper routerIP = Ipv4AddressHelper("10.3.0.0", "255.255.255.0");
	Ipv4AddressHelper senderIP = Ipv4AddressHelper("10.1.0.0", "255.255.255.0");
	Ipv4AddressHelper receiverIP = Ipv4AddressHelper("10.2.0.0", "255.255.255.0");
	

	Ipv4InterfaceContainer routerIFC, senderIFCs, receiverIFCs, leftRouterIFCs, rightRouterIFCs;

	routerIFC = routerIP.Assign(routerDevices);

	for(uint i = 0; i < numSender; ++i) {
		NetDeviceContainer senderDevice;
		senderDevice.Add(senderDevices.Get(i));
		senderDevice.Add(leftRouterDevices.Get(i));
		Ipv4InterfaceContainer senderIFC = senderIP.Assign(senderDevice);
		senderIFCs.Add(senderIFC.Get(0));
		leftRouterIFCs.Add(senderIFC.Get(1));
		senderIP.NewNetwork();

		NetDeviceContainer receiverDevice;
		receiverDevice.Add(receiverDevices.Get(i));
		receiverDevice.Add(rightRouterDevices.Get(i));
		Ipv4InterfaceContainer receiverIFC = receiverIP.Assign(receiverDevice);
		receiverIFCs.Add(receiverIFC.Get(0));
		rightRouterIFCs.Add(receiverIFC.Get(1));
		receiverIP.NewNetwork();
	}
	/********************************************************************
	PART (b)
	********************************************************************/
	/********************************************************************
		1)start 2 other flows while one is progress
		and then measure throughput and CWND of each flow at steady state
		2)Also find the max throuhput per flow
	********************************************************************/
	double durationGap = 100;
	double oneFlowStart = 0;
	double otherFlowStart = 20;
	uint port = 9000;
	uint numPackets = 10000000;
	std::string transferSpeed = "400Mbps";
		
	
	//TCP Reno from H1 to H4
	AsciiTraceHelper asciiTraceHelper;
	Ptr<OutputStreamWrapper> stream1CWND = asciiTraceHelper.CreateFileStream("application_6_h1_h4_b.cwnd");
	Ptr<OutputStreamWrapper> stream1PD = asciiTraceHelper.CreateFileStream("application_6_h1_h4_b.congestion_loss");
	Ptr<OutputStreamWrapper> stream1TP = asciiTraceHelper.CreateFileStream("application_6_h1_h4_b.tp");
	Ptr<OutputStreamWrapper> stream1GP = asciiTraceHelper.CreateFileStream("application_6_h1_h4_b.gp");
	Ptr<Socket> ns3TcpSocket1 = uniFlow(InetSocketAddress(receiverIFCs.GetAddress(0), port), port, "TcpReno", senders.Get(0), receivers.Get(0), oneFlowStart, oneFlowStart+durationGap, packetSize, numPackets, transferSpeed, oneFlowStart, oneFlowStart+durationGap);
	ns3TcpSocket1->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndChange, stream1CWND, 0));
	ns3TcpSocket1->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream1PD, 0, 1));


	std::string sink = "/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&ReceivedPacket, stream1GP, 0));
	std::string sink_ = "/NodeList/5/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&ReceivedPacketIPV4, stream1TP, 0));

	//TCP Tahoe from H2 to H5
	Ptr<OutputStreamWrapper> stream2CWND = asciiTraceHelper.CreateFileStream("application_6_h2_h5_b.cwnd");
	Ptr<OutputStreamWrapper> stream2PD = asciiTraceHelper.CreateFileStream("application_6_h2_h5_b.congestion_loss");
	Ptr<OutputStreamWrapper> stream2TP = asciiTraceHelper.CreateFileStream("application_6_h2_h5_b.tp");
	Ptr<OutputStreamWrapper> stream2GP = asciiTraceHelper.CreateFileStream("application_6_h2_h5_b.gp");
	Ptr<Socket> ns3TcpSocket2 = uniFlow(InetSocketAddress(receiverIFCs.GetAddress(1), port), port, "TcpTahoe", senders.Get(1), receivers.Get(1), otherFlowStart, otherFlowStart+durationGap, packetSize, numPackets, transferSpeed, otherFlowStart, otherFlowStart+durationGap);
	ns3TcpSocket2->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndChange, stream2CWND, 0));
	ns3TcpSocket2->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream2PD, 0, 2));

	sink = "/NodeList/6/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&ReceivedPacket, stream2GP, 0));
	sink_ = "/NodeList/6/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&ReceivedPacketIPV4, stream2TP, 0));

	//TCP WestWood from H3 to H6
	Ptr<OutputStreamWrapper> stream3CWND = asciiTraceHelper.CreateFileStream("application_6_h3_h6_b.cwnd");
	Ptr<OutputStreamWrapper> stream3PD = asciiTraceHelper.CreateFileStream("application_6_h3_h6_b.congestion_loss");
	Ptr<OutputStreamWrapper> stream3TP = asciiTraceHelper.CreateFileStream("application_6_h3_h6_b.tp");
	Ptr<OutputStreamWrapper> stream3GP = asciiTraceHelper.CreateFileStream("application_6_h3_h6_b.gp");
	Ptr<Socket> ns3TcpSocket3 = uniFlow(InetSocketAddress(receiverIFCs.GetAddress(2), port), port, "TcpWestwood", senders.Get(2), receivers.Get(2), otherFlowStart, otherFlowStart+durationGap, packetSize, numPackets, transferSpeed, otherFlowStart, otherFlowStart+durationGap);
	ns3TcpSocket3->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&CwndChange, stream3CWND, 0));
	ns3TcpSocket3->TraceConnectWithoutContext("Drop", MakeBoundCallback (&packetDrop, stream3PD, 0, 3));

	sink = "/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx";
	Config::Connect(sink, MakeBoundCallback(&ReceivedPacket, stream3GP, 0));
	sink_ = "/NodeList/7/$ns3::Ipv4L3Protocol/Rx";
	Config::Connect(sink_, MakeBoundCallback(&ReceivedPacketIPV4, stream3TP, 0));

	//p2pHR.EnablePcapAll("application_6_HR_a");
	//p2pRR.EnablePcapAll("application_6_RR_a");

	//Turning on Static Global Routing
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Ptr<FlowMonitor> flowmon;
	FlowMonitorHelper flowmonHelper;
	flowmon = flowmonHelper.InstallAll();
	Simulator::Stop(Seconds(durationGap+otherFlowStart));
	Simulator::Run();
	flowmon->CheckForLostPackets();

	//Ptr<OutputStreamWrapper> streamTP = asciiTraceHelper.CreateFileStream("application_6_b.tp");
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		/*
		*streamTP->GetStream()  << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
		*streamTP->GetStream()  << "  Tx Bytes:   " << i->second.txBytes << "\n";
		*streamTP->GetStream()  << "  Rx Bytes:   " << i->second.rxBytes << "\n";
		*streamTP->GetStream()  << "  Time        " << i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds() << "\n";
		*streamTP->GetStream()  << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";	
		*/
		if(t.sourceAddress == "10.1.0.1") {
			if(mapDrop.find(1)==mapDrop.end())
				mapDrop[1] = 0;
			*stream1PD->GetStream() << "TcpReno Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			*stream1PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream1PD->GetStream()  << "Packet Lost due to buffer overflow: " << mapDrop[1] << "\n";
			*stream1PD->GetStream()  << "Packet Lost due to Congestion: " << i->second.lostPackets - mapDrop[1] << "\n";
			*stream1PD->GetStream() << "Max throughput: " << mapMaxThroughput["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
		} else if(t.sourceAddress == "10.1.1.1") {
			if(mapDrop.find(2)==mapDrop.end())
				mapDrop[2] = 0;
			*stream2PD->GetStream() << "TcpTahoe Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			*stream2PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream2PD->GetStream()  << "Packet Lost due to buffer overflow: " << mapDrop[2] << "\n";
			*stream2PD->GetStream()  << "Packet Lost due to Congestion: " << i->second.lostPackets - mapDrop[2] << "\n";
			*stream2PD->GetStream() << "Max throughput: " << mapMaxThroughput["/NodeList/6/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
		} else if(t.sourceAddress == "10.1.2.1") {
			if(mapDrop.find(3)==mapDrop.end())
				mapDrop[3] = 0;
			*stream3PD->GetStream() << "TcpWestWood Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			*stream3PD->GetStream()  << "Net Packet Lost: " << i->second.lostPackets << "\n";
			*stream3PD->GetStream()  << "Packet Lost due to buffer overflow: " << mapDrop[3] << "\n";
			*stream3PD->GetStream()  << "Packet Lost due to Congestion: " << i->second.lostPackets - mapDrop[3] << "\n";
			*stream3PD->GetStream() << "Max throughput: " << mapMaxThroughput["/NodeList/7/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
		}
	}

	//flowmon->SerializeToXmlFile("application_6_b.flowmon", true, true);
	Simulator::Destroy();

}

int main(int argc, char **argv) {
	CommandLine cmd;
	std::string type;
  	cmd.AddValue ("part", "Which part to run?", type);
  	cmd.Parse (argc, argv);
		
	if(atoi(type.c_str()) == 1)
		partAC();
	else
		partBC();
}