#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <vector>
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include <ctime>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("APCC");

static Ptr<OutputStreamWrapper> rttStream;
static void RttTracer (Time oldval, Time newval) {
    *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}
static void TraceRtt (string rtt_tr_file_name) {
    AsciiTraceHelper ascii;
    rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

ofstream qlenOutput;
void DevicePacketsInQueueTrace (Ptr<QueueDisc> root, uint32_t oldValue, uint32_t newValue) {
    uint32_t qlen = newValue + root->GetQueueDiscClass (0)->GetQueueDisc ()->GetCurrentSize ().GetValue ();
    qlenOutput << Simulator::Now ().GetSeconds () << " " << qlen << std::endl;       
}

uint64_t lastSendBytes[7] = {0, 0, 0, 0, 0, 0, 0};
uint64_t tmp[7];
static Ptr<OutputStreamWrapper> thputStream[7];
void RateByPtpNetDevice(Ptr<PointToPointNetDevice> device, int no, double interval){
	Time time = Seconds(interval);
	tmp[no] = device->m_sendBytes - lastSendBytes[no];
	lastSendBytes[no] = device->m_sendBytes;
	double rate = tmp[no] * 8/ (1000 * 1000  * time.GetSeconds());
	*thputStream[no]->GetStream () << Simulator::Now ().GetSeconds () << '\t' << rate << "Mbps" << endl;
	Simulator::Schedule(time, &RateByPtpNetDevice, device, no, interval);
}
static void TraceThput (string thput_tr_file_name, Ptr<PointToPointNetDevice> device, int no, double interval) {
    AsciiTraceHelper ascii;
    thputStream[no] = ascii.CreateFileStream (thput_tr_file_name.c_str ());
    RateByPtpNetDevice(device, no, interval);
}


int main (int argc, char *argv[]) {   
    uint32_t cc_mode = 3;

    // Config::SetDefault ("ns3::WifiMacQueue::MaxSize", QueueSizeValue (QueueSize ("100p")));  // default 500p 
    // Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (MilliSeconds (1000)));
    std::string resultPath = "result_mf_v3/";
    if (cc_mode == 0) {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpBbr"));        
    }
    else if (cc_mode == 1) {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpCubic")); 
        Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
        Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
        Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1)); // instant queue
    }    
    else if (cc_mode == 2) {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno")); 
        Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
    }
    else if (cc_mode == 3) {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno")); 
    }
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (67108864));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (67108864));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (200)));

    // Start create topo
    string rate_1 = "100000Mbps";  // the rate of "remote server" to core switch 
    string rate_2 = "40000Mbps";  // the rate of core switch to convergence switch 
    string rate_3 = "10000Mbps";  // the rate of convergence switch to access switch 
    string rate_4 = "1000Mbps";  // the rate of access switch to ap
    string channel_delay = "0.15ms";

    // create nodes
    NodeContainer remote_servers;
    uint32_t server_num = 4;
    remote_servers.Create(server_num);
    NodeContainer core_switches;
    core_switches.Create(1);
    NodeContainer convergence_switches;
    convergence_switches.Create(1);
    NodeContainer access_switches;
    access_switches.Create(1);
    NodeContainer ap;
    uint32_t ap_num = 1;
    ap.Create(ap_num);
    vector<NodeContainer> wifi_nodes(ap_num);
    for(uint32_t i = 0; i < ap_num; i++) {
        wifi_nodes[i].Create(2);     
    }

    // install stack
    InternetStackHelper stack;
	  stack.Install(remote_servers);
	  stack.Install(core_switches);
	  stack.Install(convergence_switches);
	  stack.Install(access_switches);
	  stack.Install(ap);
    for(uint32_t i = 0; i < ap_num; i++) {
        stack.Install(wifi_nodes[i]);
    }

    // create wired connect        
    PointToPointHelper p2p_1;
    p2p_1.SetDeviceAttribute("DataRate", StringValue(rate_1));
    p2p_1.SetChannelAttribute("Delay", StringValue("9ms"));
    vector<NetDeviceContainer> p2p_1_devices(server_num);
    for(uint32_t i = 0; i < server_num; i++) {
        p2p_1_devices[i] = p2p_1.Install(remote_servers.Get(i), core_switches.Get(0));
    }
    PointToPointHelper p2p_2;
    p2p_2.SetDeviceAttribute("DataRate", StringValue(rate_2));
    p2p_2.SetChannelAttribute("Delay", StringValue(channel_delay));
    NetDeviceContainer p2p_2_devices;
    p2p_2_devices = p2p_2.Install(core_switches.Get(0), convergence_switches.Get(0));    
    PointToPointHelper p2p_3;
    p2p_3.SetDeviceAttribute("DataRate", StringValue(rate_3));
    p2p_3.SetChannelAttribute("Delay", StringValue(channel_delay));
    NetDeviceContainer p2p_3_devices;
    p2p_3_devices = p2p_3.Install(convergence_switches.Get(0), access_switches.Get(0));
    PointToPointHelper p2p_4;
    p2p_4.SetDeviceAttribute("DataRate", StringValue(rate_4));
    p2p_4.SetChannelAttribute("Delay", StringValue(channel_delay));
    vector<NetDeviceContainer> access_devices(ap_num);
    for(uint32_t i = 0; i < ap_num; i++) {
        access_devices[i] = p2p_4.Install(access_switches.Get(0), ap.Get(i));
    }

    // create wifi connect
    vector<YansWifiChannelHelper> wifi_channel(ap_num);
    vector<YansWifiPhyHelper> wifi_phy(ap_num);
    for(uint32_t i = 0; i < ap_num; i++) {
        wifi_channel[i] = YansWifiChannelHelper::Default();
        wifi_phy[i].SetChannel (wifi_channel[i].Create ());

        //transmission power: 27dBm
        wifi_phy[i].Set("TxPowerStart", DoubleValue(27));
        wifi_phy[i].Set("TxPowerEnd", DoubleValue(27));
        wifi_phy[i].Set("TxPowerLevels", UintegerValue(1));
    }    

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211ax_5GHZ);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("HeMcs11"),
                                "ControlMode", StringValue ("HeMcs11"),
                                "RateChange",BooleanValue(true));
    // wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");
    WifiMacHelper wifi_mac;
    vector<NetDeviceContainer> wifi_nodes_devices(ap_num);
    vector<NetDeviceContainer> ap_devices(ap_num);
    for(uint32_t i = 0; i < ap_num; i++) {
        string ssid_name = "ap" + std::to_string(i);
        Ssid ssid = Ssid(ssid_name);
        wifi_mac.SetType("ns3::StaWifiMac",
                    "Ssid", SsidValue(ssid));
        wifi_nodes_devices[i] = wifi.Install(wifi_phy[i], wifi_mac, wifi_nodes[i]);
        wifi_mac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid));
        ap_devices[i] = wifi.Install(wifi_phy[i], wifi_mac, ap.Get(i));
    }
    // Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::WifiMac/QosSupported", BooleanValue (false));
    // std::cout<<DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(ap_devices[0].Get(0))->GetMac())->GetQosSupported()<<std::endl;
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelNumber", UintegerValue (50)); // set channel width 160MHz    
    // std::cout<<DynamicCast<WifiNetDevice>(ap_devices[0].Get(0))->GetPhy()->GetChannelWidth()<<std::endl;   
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval", TimeValue (NanoSeconds (800)));


    // mobility.
    vector<MobilityHelper> mobility(ap_num);
    vector<Ptr<ListPositionAllocator>> positionAlloc(ap_num);
    for(uint32_t i = 0; i < ap_num; i++) {
        positionAlloc[i] = CreateObject<ListPositionAllocator> ();
        positionAlloc[i]->Add (Vector (0.0, i*50.0, 0.0)); 
        positionAlloc[i]->Add (Vector (3.0, i*50.0, 0.0)); 
        positionAlloc[i]->Add (Vector (-3.0, i*50.0, 0.0));
        mobility[i].SetPositionAllocator (positionAlloc[i]);
        mobility[i].SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility[i].Install (ap.Get(i));
        mobility[i].Install (wifi_nodes[i]);
    }
    
    // install queue disc
    // To install a queue disc other than the default one, it is necessary to install such queue disc before an IP address is assigned to the device
    
    // TrafficControlHelper tchWifi;
    // tchWifi.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue ("5p"));
    // vector<QueueDiscContainer> qdiscs(ap_num);
    // for(uint32_t i = 0; i < ap_num; i++) {
    //     qdiscs[i] = tchWifi.Install(ap_devices[i]);
    // }

    TrafficControlHelper tchAcessSw;
    tchAcessSw.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue ("1633p"));
    tchAcessSw.Install(access_devices[0].Get(0));

    TrafficControlHelper tch;
    uint16_t handle = tch.SetRootQueueDisc ("ns3::MqQueueDisc");
    TrafficControlHelper::ClassIdList cls = tch.AddQueueDiscClasses (handle, 4, "ns3::QueueDiscClass");
    if (cc_mode == 0) {
        tch.AddChildQueueDiscs (handle, cls, "ns3::FifoQueueDisc");
    }
    else if (cc_mode == 1) {
        tch.AddChildQueueDiscs (handle, cls, "ns3::RedQueueDisc");
    }   
    else if (cc_mode == 2) {
        tch.AddChildQueueDiscs (handle, cls, "ns3::FifoQueueDisc");
    } 
    else if (cc_mode == 3) {
        tch.AddChildQueueDiscs (handle, cls, "ns3::FifoQueueDisc");
    } 
    tch.Install (ap_devices[0]);
    Ptr<QueueDisc> root_qdisc = ap.Get(0)->GetObject<TrafficControlLayer> ()->GetRootQueueDiscOnDevice(ap_devices[0].Get(0));
    root_qdisc->GetQueueDiscClass (0)->GetQueueDisc () -> SetMaxSize (QueueSize("7500p"));
    if (cc_mode == 0) {
        
    }
    else if (cc_mode == 1) {
        root_qdisc->GetQueueDiscClass (0)->GetQueueDisc () -> SetAttribute ("MinTh", DoubleValue (350));
        root_qdisc->GetQueueDiscClass (0)->GetQueueDisc () -> SetAttribute ("MaxTh", DoubleValue (350));        
    }
    else if (cc_mode == 2) {
        Ptr<TrafficControlLayer> tc = ap.Get(0)->GetObject<TrafficControlLayer> ();
        tc->SetAttribute("Ccmode", UintegerValue (2));
        tc->SetAttribute("Kmin", UintegerValue (450));  // 0.31*650Mbps*20ms/1500Bpp=336packet
    }
    else if (cc_mode == 3) {
        Ptr<TrafficControlLayer> tc = ap.Get(0)->GetObject<TrafficControlLayer> ();
        tc->SetAttribute("Ccmode", UintegerValue (3));
        tc->SetAttribute("Kmin", UintegerValue (20));
        tc->SetAttribute("Tau", UintegerValue (80));
        tc->SetAttribute("DeviceRate", UintegerValue (630));
        tc->SetAttribute("IsWired", BooleanValue (false));
    }


    // assign IP address
    Ipv4AddressHelper address;
    vector<Ipv4InterfaceContainer> p2p_1_interfaces(server_num);
    for(uint32_t i = 0; i < server_num; i++) {
        std::ostringstream subnet;
	    subnet<<"192.168.1"<< i + 10 <<".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        p2p_1_interfaces[i] = address.Assign(p2p_1_devices[i]);
    }
    address.SetBase ("192.168.201.0", "255.255.255.0");
    Ipv4InterfaceContainer p2p_2_interfaces = address.Assign(p2p_2_devices);
    address.SetBase ("192.168.202.0", "255.255.255.0");
    Ipv4InterfaceContainer p2p_3_interfaces = address.Assign(p2p_3_devices);
    vector<Ipv4InterfaceContainer> access_interfaces(ap_num);
    for(uint32_t i = 0; i < ap_num; i++) {
        std::ostringstream subnet;
	    subnet<<"192.168.2"<< i + 10 <<".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        access_interfaces[i] = address.Assign(access_devices[i]);
    }
    vector<Ipv4InterfaceContainer> ap_interfaces(ap_num);
    vector<Ipv4InterfaceContainer> wifi_nodes_interfaces(ap_num);
    for(uint32_t i = 0; i < ap_num; i++) {
        std::ostringstream subnet;
	    subnet<<"192.168.2"<< i + 30 <<".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        ap_interfaces[i] = address.Assign(ap_devices[i]);
        wifi_nodes_interfaces[i] = address.Assign(wifi_nodes_devices[i]);
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    // End create topo

    // Start create flow
    double simulationTime = 40;  // seconds
    double appStartTime = 2.0;
    uint32_t flow_num = server_num;
    vector<ApplicationContainer> sinkAppA(flow_num);
    vector<ApplicationContainer> sourceAppA(flow_num);
    for(uint32_t i = 0; i < flow_num; i++) {
        uint16_t port = 50000 + i;
        PacketSinkHelper sinkHelperA ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
        sinkAppA[i] = sinkHelperA.Install (wifi_nodes[0].Get(0));
        sinkAppA[i].Start (Seconds(appStartTime - 1 + i*3));
        sinkAppA[i].Stop (Seconds(simulationTime + appStartTime));
        InetSocketAddress remoteAddressA = InetSocketAddress (wifi_nodes_interfaces[0].GetAddress(0), port);
        // remoteAddressA.SetTos(0x14);
        BulkSendHelper sourceHelperA ("ns3::TcpSocketFactory", remoteAddressA);
        sourceHelperA.SetAttribute ("SendSize", UintegerValue (1000));
        sourceHelperA.SetAttribute ("MaxBytes", UintegerValue (0));
        sourceAppA[i].Add (sourceHelperA.Install (remote_servers.Get(i)));
        sourceAppA[i].Start (Seconds(appStartTime + i*3));
        sourceAppA[i].Stop (Seconds(simulationTime + appStartTime));
    }
    // End create flow

    // Start Trace
    double intvl = 0.1;
    for(uint32_t i = 0; i < server_num; i++) {
        Ptr<PointToPointNetDevice> device1 = DynamicCast<PointToPointNetDevice>(p2p_1_devices[i].Get(0));        
        Simulator::Schedule(Seconds(intvl), &TraceThput, resultPath+"thput"+to_string(i)+".data", device1, i, intvl);
    }

    // void RateByWifiNetDevice(Ptr<WifiNetDevice> device, int no, double interval);
    Ptr<WifiNetDevice> device2 = DynamicCast<WifiNetDevice>(ap_devices[0].Get(0));
    // Simulator::Schedule(Seconds(intvl), &RateByWifiNetDevice, device2, 1, intvl);

    Simulator::Schedule (Seconds (appStartTime + 0.00001), &TraceRtt, resultPath+"rtt.data");
    qlenOutput.open (resultPath+"qlen.data"); 
    Ptr<ns3::WifiMacQueue> queue = DynamicCast<RegularWifiMac>(device2->GetMac())->GetTxopQueue(AC_BE);
    queue->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&DevicePacketsInQueueTrace, root_qdisc));
    
    // void printCurrentSize(Ptr<QueueDisc> root);    
    // Simulator::Schedule(Seconds(0.1), &printCurrentSize, root);

    // End Trace

    // Start Run Simulation
    Simulator::Stop(Seconds(simulationTime + appStartTime + 0.00001));
    time_t start = time(nullptr);  
    Simulator::Run();
    time_t end = time(nullptr);  
    std::cout<<"Simulate time : "<<end-start<<" seconds"<<std::endl;
    Simulator::Destroy();
    // End Run Simulation

    uint64_t rxBytes;
    double throughput;
    double total_thput = 0.0;
    for(uint32_t i = 0; i < flow_num; i++) {
        rxBytes = DynamicCast<PacketSink> (sinkAppA[i].Get (0))->GetTotalRx ();  // pure data: 1448B per packet
        throughput = (rxBytes * 8) / (simulationTime * 1000000.0); //Mbit/s
        std::cout<<"Receive APP"<<i<<" throughput : "<< throughput <<" Mbps"<<std::endl;
        total_thput += throughput;
    }
     std::cout<<"Receive APP TOTAL throughput : "<< total_thput <<" Mbps"<<std::endl;


    return 0;
}

void printCurrentSize(Ptr<QueueDisc> root){
    Time time = Seconds(0.001);
    cout<<Simulator::Now ().GetSeconds () << '\t' <<root->GetQueueDiscClass (0)->GetQueueDisc ()->GetCurrentSize ()<<endl;
    Simulator::Schedule(time, &printCurrentSize, root);
}

void RateByWifiNetDevice(Ptr<WifiNetDevice> device, int no, double interval){
	Time time = Seconds(interval);
    uint64_t sendBytes = DynamicCast<RegularWifiMac>(device->GetMac())->GetTxopQueue(AC_BE)->m_sendBytes;
	tmp[no] = sendBytes - lastSendBytes[no];
	lastSendBytes[no] = sendBytes;
	double rate = tmp[no] * 8/ (1000 * 1000  * time.GetSeconds());
	cout << Simulator::Now ().GetSeconds () << '\t' << rate << "Mbps" << endl;
	Simulator::Schedule(time, &RateByWifiNetDevice, device, no, interval);
}
