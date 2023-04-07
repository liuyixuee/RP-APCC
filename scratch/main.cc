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
#include <sys/stat.h>
#include <sys/types.h>
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

//trace cwnd
static Ptr<OutputStreamWrapper> cwndStream_0;
static void CwndTracer_0 (unsigned int oldval, unsigned int newval) {
    *cwndStream_0->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
}
static void TraceCwnd (string cwnd_tr_file_name_0) {
    AsciiTraceHelper ascii_0;
    cwndStream_0 = ascii_0.CreateFileStream (cwnd_tr_file_name_0.c_str ());
    //New Reno
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer_0));
}
static Ptr<OutputStreamWrapper> rwndStream_0;
static void RwndTracer_0 (unsigned int oldval, unsigned int newval) {
    *rwndStream_0->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
}
static void TraceRwnd (string rwnd_tr_file_name_0) {
    AsciiTraceHelper ascii_0;
    rwndStream_0 = ascii_0.CreateFileStream (rwnd_tr_file_name_0.c_str ());
    //New Reno
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RWND", MakeCallback (&RwndTracer_0));
}


ofstream qlenOutput;
void DevicePacketsInQueueTrace (Ptr<QueueDisc> root, uint32_t oldValue, uint32_t newValue) {
    uint32_t qlen = newValue + root->GetQueueDiscClass (0)->GetQueueDisc ()->GetCurrentSize ().GetValue ();
    qlenOutput << Simulator::Now ().GetSeconds () << " " << qlen << std::endl;       
}

uint64_t lastSendBytes[5] = {0, 0, 0, 0, 0};
uint64_t tmp[5];
static Ptr<OutputStreamWrapper> thputStream;
void RateByPtpNetDevice(Ptr<PointToPointNetDevice> device, int no, double interval){
	Time time = Seconds(interval);
	tmp[no] = device->m_sendBytes - lastSendBytes[no];
	lastSendBytes[no] = device->m_sendBytes;
	double rate = tmp[no] * 8/ (1000 * 1000  * time.GetSeconds());
	*thputStream->GetStream () << Simulator::Now ().GetSeconds () << '\t' << rate << "Mbps" << endl;
	Simulator::Schedule(time, &RateByPtpNetDevice, device, no, interval);
}
static void TraceThput (string thput_tr_file_name, Ptr<PointToPointNetDevice> device, int no, double interval) {
    AsciiTraceHelper ascii;
    thputStream = ascii.CreateFileStream (thput_tr_file_name.c_str ());
    RateByPtpNetDevice(device, no, interval);
}

uint64_t lastRxBytes[5] = {0, 0, 0, 0, 0};
uint64_t tmp_appBytes[5];
static Ptr<OutputStreamWrapper> appThputStream;
void RateByApp(Ptr<PacketSink> app, int no, double interval){
	Time time = Seconds(interval);
	tmp_appBytes[no] = app->GetTotalRx () - lastRxBytes[no];
	lastRxBytes[no] = app->GetTotalRx ();
	double rate = tmp_appBytes[no] * 8/ (1000 * 1000  * time.GetSeconds());
	*appThputStream->GetStream () << Simulator::Now ().GetSeconds () << '\t' << rate << "Mbps" << endl;
	Simulator::Schedule(time, &RateByApp, app, no, interval);
}
static void TraceAppThput (string thput_tr_file_name, Ptr<PacketSink> app, int no, double interval) {
    AsciiTraceHelper ascii;
    appThputStream = ascii.CreateFileStream (thput_tr_file_name.c_str ());
    RateByApp(app, no, interval);
}


//dequeue rate record
ofstream dequeueStream;
static Ptr<OutputStreamWrapper> dequeue_thputStream;
AsciiTraceHelper ascii_dequeue;


uint64_t out_packet=0,last_outpacket=0;
void DataRateInQueueTrace (double interval)
{
    
    Time time = Seconds(interval);
    uint64_t tmp=out_packet-last_outpacket;
    last_outpacket=out_packet;
    double rate = tmp * 8/ (1000 * 1000  * time.GetSeconds());
    *dequeue_thputStream->GetStream()<<Simulator::Now().GetSeconds() << " "<<rate<<"Mbps"<<std::endl;
    Simulator::Schedule(time, &DataRateInQueueTrace, interval);
}


    
// }
void
PacketDequeued (Ptr<ns3::QueueDiscItem const> item)
{
  //m_nPackets--;
  out_packet+= item->GetSize ();
  //cout<<"size "<<item->GetSize()<<" total_output: "<<out_packet<<endl;
  
  if(last_outpacket==0)
  {
    DataRateInQueueTrace(0.02);
    cout<<"begin dequeue trace"<<endl;
  } 
}
uint64_t TxdropByte = 0,RxdropByte = 0;
static void TxDropTracer(Ptr<const Packet> packet)
{
    TxdropByte +=packet->GetSize();
}
static void RxDropTracer(Ptr<const Packet> packet, WifiPhyRxfailureReason r)
{
    RxdropByte +=packet->GetSize();
}

int main (int argc, char *argv[]) {   
    CommandLine cmd;
    uint32_t cc_mode = 2;
    std::string resultPath = "result_udp/";
    cmd.AddValue("cc_mode",  "cc mode", cc_mode);
    cmd.AddValue("outputPath", "path of output file", resultPath);
    cmd.Parse (argc, argv);
    string dequeue_path = resultPath + "dequeue_rate.data";
    dequeue_thputStream = ascii_dequeue.CreateFileStream (dequeue_path.c_str ());
    dequeueStream.open(resultPath + "dequeue_rate.data");
    
    // int isCreate = mkdir((char*)resultPath.c_str(), S_IRWXU| S_IRWXG | S_IRWXO);
    // if( isCreate )
    //     cout<<"create path "<<resultPath<<endl;
    // else
    //     cout<<"create failed: "<<isCreate <<" of path "<<resultPath<<endl;
    //LogComponentEnable ("TrafficControlLayer", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("ApWifiMac", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("FqFifoQueueDisc", LOG_LEVEL_FUNCTION);
    //LogComponentEnable("FifoQueueDisc",LOG_LEVEL_FUNCTION);
    //LogComponentEnable("QueueDisc",LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("WifiMacQueue", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("QueueDisc", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("TcpSocketBase", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("TcpTxBuffer", LOG_LEVEL_FUNCTION);
    //LogComponentEnable ("Packet", LOG_LEVEL_FUNCTION);
    // Config::SetDefault ("ns3::WifiMacQueue::MaxSize", QueueSizeValue (QueueSize ("100p")));  // default 500p 
    // Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (MilliSeconds (1000)));

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
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpCubic")); 
        Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
    }
    else if (cc_mode == 3) {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno")); 
    }
    else if (cc_mode == 4) {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno")); 
    }
    else if (cc_mode == 5) {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpCubic")); 
        Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
    }    
    else if(cc_mode == 6)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpCubic")); 
        Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
        Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
        Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1)); // instant queue
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
    remote_servers.Create(1);
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
    NetDeviceContainer p2p_1_devices;
    p2p_1_devices = p2p_1.Install(remote_servers.Get(0), core_switches.Get(0));
    
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
        access_devices[i] = p2p_4.Install( ap.Get(i),access_switches.Get(0));
    }
    

    // create wifi connect
    vector<YansWifiChannelHelper> wifi_channel(ap_num);
    vector<YansWifiPhyHelper> wifi_phy(ap_num);
    if(1)
    {

        for(uint32_t i = 0; i < ap_num; i++) {
            wifi_channel[i] = YansWifiChannelHelper::Default();
            wifi_phy[i].SetChannel (wifi_channel[i].Create ());

            //transmission power: 27dBm
            wifi_phy[i].Set("TxPowerStart", DoubleValue(27));
            wifi_phy[i].Set("TxPowerEnd", DoubleValue(27));
            wifi_phy[i].Set("TxPowerLevels", UintegerValue(1));
        }    
    }
    else{
        
        for(uint32_t i = 0; i < ap_num; i++) {
            wifi_channel[i] = YansWifiChannelHelper::Default();
            // wifi_channel[i].AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(10));
            wifi_channel[i].AddPropagationLoss("ns3::RandomPropagationLossModel");
            wifi_phy[i].SetChannel (wifi_channel[i].Create ());

            //transmission power: 27dBm
            wifi_phy[i].Set("TxPowerStart", DoubleValue(27));
            wifi_phy[i].Set("TxPowerEnd", DoubleValue(27));
            wifi_phy[i].Set("TxPowerLevels", UintegerValue(1));
        }    
    }
    

    WifiHelper wifi;
    if(0)
    {
        wifi.SetStandard (WIFI_STANDARD_80211n_2_4GHZ);
        wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
    }
    else{
        wifi.SetStandard (WIFI_STANDARD_80211ax_5GHZ);
        wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("HeMcs11"),
                                "ControlMode", StringValue ("HeMcs11"),
                                "RateChange",BooleanValue(false)
                                );
    }
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
    uint16_t mobility_mode=2;
    if(mobility_mode == 0) {
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
    }
    else if(mobility_mode == 1) {
        vector<MobilityHelper> mobility(ap_num);
        //vector<MobilityHelper> mobility_node(ap_num);
        vector<Ptr<ListPositionAllocator>> positionAlloc(ap_num);
        for(uint32_t i = 0; i < ap_num; i++) {
            positionAlloc[i] = CreateObject<ListPositionAllocator> ();
            positionAlloc[i]->Add (Vector (0.0, i*50.0, 0.0)); 
            positionAlloc[i]->Add (Vector (3.0, i*50.0, 0.0)); 
            positionAlloc[i]->Add (Vector (-3.0, i*50.0, 0.0));
            mobility[i].SetPositionAllocator (positionAlloc[i]);
            mobility[i].SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                           "Speed", StringValue ("ns3::UniformRandomVariable[Min=0.1|Max=3]"),
                           "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                           "PositionAllocator", PointerValue (positionAlloc[i]));
            mobility[i].Install (ap.Get(i));
            mobility[i].Install (wifi_nodes[i]);
        }
    }
    else if(mobility_mode == 2)
    {
        MobilityHelper mobility1;
        mobility1.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(0.0),
                                    "MinY", DoubleValue(0.0),
                                    "DeltaX", DoubleValue(0.0),
                                    "DeltaY", DoubleValue(0.0),
                                    "GridWidth", UintegerValue(1),
                                    "LayoutType", StringValue("RowFirst"));
        mobility1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility1.Install(ap.Get(0));

        MobilityHelper mobility2;
        mobility2.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(-10.0),
                                    "MinY", DoubleValue(0.0),
                                    "DeltaX", DoubleValue(0.0),
                                    "DeltaY", DoubleValue(0.0),
                                    "GridWidth", UintegerValue(1),
                                    "LayoutType", StringValue("RowFirst"));
        mobility2.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility2.Install(wifi_nodes[0].Get(0));

        MobilityHelper mobility3;
        mobility3.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(10.0),
                                    "MinY", DoubleValue(0.0),
                                    "DeltaX", DoubleValue(0.0),
                                    "DeltaY", DoubleValue(0.0),
                                    "GridWidth", UintegerValue(1),
                                    "LayoutType", StringValue("RowFirst"));

        mobility3.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                "Mode", StringValue  ("Distance"),
                                "Distance", DoubleValue(1.0),
                                "Speed", StringValue("ns3::ConstantRandomVariable[Constant=100.0]"),
                                "Bounds", StringValue("-9|11|-0.5|0.5"));
        mobility3.Install(wifi_nodes[0].Get(1));
    }
    else if(mobility_mode==3)
    {
        vector<MobilityHelper> mobility(ap_num);
        vector<Ptr<ListPositionAllocator>> positionAlloc(ap_num);
        for(uint32_t i = 0; i < ap_num; i++) {
            positionAlloc[i] = CreateObject<ListPositionAllocator> ();
            positionAlloc[i]->Add (Vector (0.0, i*50.0, 0.0)); 
            positionAlloc[i]->Add (Vector (3.0, i*50.0, 0.0)); 
            positionAlloc[i]->Add (Vector (-3.0, i*50.0, 0.0));
            mobility[i].SetPositionAllocator (positionAlloc[i]);
            // mobility[i].SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobility[i].SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
            "Bounds", RectangleValue (Rectangle (-10, 10, -10, 10)));
            
            mobility[i].Install (ap.Get(i));
            mobility[i].Install (wifi_nodes[i]);
        }
    }
    


    TrafficControlHelper tchAcessSw;
    //tchAcessSw.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue ("1633p"));
    if (cc_mode == 4) {
        tchAcessSw.SetRootQueueDisc("ns3::FqFifoQueueDisc");
    }
    else {
        tchAcessSw.SetRootQueueDisc("ns3::FifoQueueDisc");
    }
    
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
    else if (cc_mode ==4)
    {
        tch.AddChildQueueDiscs (handle, cls, "ns3::FqFifoQueueDisc");
        
    }
    else if (cc_mode ==5)
    {
        tch.AddChildQueueDiscs (handle, cls, "ns3::FqFifoQueueDisc");
        
    }
    else if( cc_mode == 6)
    {
        tch.AddChildQueueDiscs (handle, cls, "ns3::FifoQueueDisc");
    }
    tch.Install (ap_devices[0]);
    Ptr<QueueDisc> root_qdisc = ap.Get(0)->GetObject<TrafficControlLayer> ()->GetRootQueueDiscOnDevice(ap_devices[0].Get(0));
    root_qdisc->GetQueueDiscClass (0)->GetQueueDisc () -> SetMaxSize (QueueSize("19500p"));
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
        tc->SetAttribute("DeviceRate", UintegerValue (630));
        tc->SetAttribute("IsWired", BooleanValue (false));
    }
    else if (cc_mode == 3) {
        Ptr<TrafficControlLayer> tc = ap.Get(0)->GetObject<TrafficControlLayer> ();
        tc->SetAttribute("Ccmode", UintegerValue (3));
        tc->SetAttribute("Kmin", UintegerValue (20));
        tc->SetAttribute("Tau", UintegerValue (80));
        tc->SetAttribute("DeviceRate", UintegerValue (630));
        tc->SetAttribute("IsWired", BooleanValue (false));
    }
    else if( cc_mode == 4) {
        Ptr<TrafficControlLayer> tc = ap.Get(0)->GetObject<TrafficControlLayer> ();
        tc->SetAttribute("Ccmode", UintegerValue (4));
        tc->SetAttribute("Kmin", UintegerValue (20));
        tc->SetAttribute("Tau", UintegerValue (80));
        tc->SetAttribute("DeviceRate", UintegerValue (630));
        tc->SetAttribute("IsWired", BooleanValue (false));
        tc->SetAttribute("ap_uplink",BooleanValue(true));
    }
    else if (cc_mode == 5) {
        Ptr<TrafficControlLayer> tc = ap.Get(0)->GetObject<TrafficControlLayer> ();
        tc->SetAttribute("Ccmode", UintegerValue (5));
        tc->SetAttribute("Kmin", UintegerValue (20));
        tc->SetAttribute("Tau", UintegerValue (80));
        tc->SetAttribute("DeviceRate", UintegerValue (630));
        tc->SetAttribute("IsWired", BooleanValue (false));
        tc->SetAttribute("ap_uplink",BooleanValue(true));
    }  
    else if(cc_mode == 6)
    {
        Ptr<TrafficControlLayer> tc = ap.Get(0)->GetObject<TrafficControlLayer> ();
        tc->SetAttribute("Ccmode", UintegerValue (3));
        tc->SetAttribute("Kmin", UintegerValue (20));
        tc->SetAttribute("Tau", UintegerValue (80));
        tc->SetAttribute("DeviceRate", UintegerValue (630));
        tc->SetAttribute("IsWired", BooleanValue (false));
    }  


    // assign IP address
    Ipv4AddressHelper address;
    address.SetBase ("192.168.200.0", "255.255.255.0");
    Ipv4InterfaceContainer p2p_1_interfaces = address.Assign(p2p_1_devices);
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
    double simulationTime = 10;  // seconds
    double appStartTime = 2.0;
    uint32_t flow_num = 2;
    vector<ApplicationContainer> sinkAppA(flow_num);
    vector<ApplicationContainer> sourceAppA(flow_num);
    if(0)
    {
        for(uint32_t i = 0; i < flow_num; i++) {
            uint16_t port = 50000 + i;
            PacketSinkHelper sinkHelperA ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
            sinkAppA[i] = sinkHelperA.Install (wifi_nodes[0].Get(i%2));
            sinkAppA[i].Start (Seconds(appStartTime - 1+ i*5));
            sinkAppA[i].Stop (Seconds(simulationTime + appStartTime));
            InetSocketAddress remoteAddressA = InetSocketAddress (wifi_nodes_interfaces[0].GetAddress(0), port);
            // remoteAddressA.SetTos(0x14);
            BulkSendHelper sourceHelperA ("ns3::TcpSocketFactory", remoteAddressA);
            sourceHelperA.SetAttribute ("SendSize", UintegerValue (1000));
            sourceHelperA.SetAttribute ("MaxBytes", UintegerValue (0));
            sourceAppA[i].Add (sourceHelperA.Install (remote_servers.Get(0)));
            sourceAppA[i].Start (Seconds(appStartTime+ i*5));
            sourceAppA[i].Stop (Seconds(simulationTime + appStartTime));
        }
    }

    else
    {
        for(uint32_t i = 0; i < flow_num; i++) {
            uint16_t port = 50000 + i;
            if(i==0)
            {
                PacketSinkHelper sinkHelperA ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
                sinkAppA[i] = sinkHelperA.Install (wifi_nodes[0].Get(i%2));
                sinkAppA[i].Start (Seconds(appStartTime - 1+ i*5));
                sinkAppA[i].Stop (Seconds(simulationTime + appStartTime));
                InetSocketAddress remoteAddressA = InetSocketAddress (wifi_nodes_interfaces[0].GetAddress(0), port);
                // remoteAddressA.SetTos(0x14);
                BulkSendHelper sourceHelperA ("ns3::TcpSocketFactory", remoteAddressA);
                sourceHelperA.SetAttribute ("SendSize", UintegerValue (1000));
                sourceHelperA.SetAttribute ("MaxBytes", UintegerValue (0));
                sourceAppA[i].Add (sourceHelperA.Install (remote_servers.Get(0)));
                sourceAppA[i].Start (Seconds(appStartTime+ i*5));
                sourceAppA[i].Stop (Seconds(simulationTime + appStartTime));
            }
            else{
                PacketSinkHelper sinkHelperA ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
                sinkAppA[i] = sinkHelperA.Install (remote_servers.Get(0));
                sinkAppA[i].Start (Seconds(appStartTime - 1+ i*2));
                sinkAppA[i].Stop (Seconds(simulationTime + appStartTime));
                InetSocketAddress remoteAddressA = InetSocketAddress (p2p_1_interfaces.GetAddress(0), port);
                // remoteAddressA.SetTos(0x14);
                OnOffHelper sourceHelperA ("ns3::UdpSocketFactory", remoteAddressA);
                sourceHelperA.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
                sourceHelperA.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
                sourceAppA[i].Add (sourceHelperA.Install (wifi_nodes[0].Get(i%2)));
                sourceAppA[i].Start (Seconds(appStartTime+ i*2));
                sourceAppA[i].Stop (Seconds(simulationTime + appStartTime));
            }
        }
    }
    // End create flow

    // Start Trace
    Ptr<PointToPointNetDevice> device1 = DynamicCast<PointToPointNetDevice>(p2p_1_devices.Get(0));
    double intvl = 0.01;


    Simulator::Schedule(Seconds(intvl), &TraceThput, resultPath+"thput.data", device1, 0, intvl);
    vector<Ptr<PacketSink>> apps(flow_num) ;
    for(uint32_t i = 0; i< flow_num; i++) apps[i] = DynamicCast<PacketSink> (sinkAppA[i].Get (0));
    for(uint32_t i = 0; i< flow_num; i++) Simulator::Schedule(Seconds(intvl), &TraceAppThput, resultPath+"appthput"+to_string(i)+".data", apps[i], i, intvl);
    // void RateByWifiNetDevice(Ptr<WifiNetDevice> device, int no, double interval);
    Ptr<WifiNetDevice> device2 = DynamicCast<WifiNetDevice>(ap_devices[0].Get(0));
    // Simulator::Schedule(Seconds(intvl), &RateByWifiNetDevice, device2, 1, intvl);

    Simulator::Schedule (Seconds (appStartTime + 0.00001), &TraceRtt, resultPath+"rtt.data");
    Simulator::Schedule (Seconds (appStartTime + 0.00001), &TraceCwnd, resultPath+"cwnd.data");
    Simulator::Schedule (Seconds (appStartTime + 0.00001), &TraceRwnd, resultPath+"rwnd.data");
    qlenOutput.open (resultPath+"qlen.data"); 
    Ptr<ns3::WifiMacQueue> queue = DynamicCast<RegularWifiMac>(device2->GetMac())->GetTxopQueue(AC_BE);
    queue->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&DevicePacketsInQueueTrace, root_qdisc));
    root_qdisc->TraceConnectWithoutContext("Dequeue",MakeCallback(PacketDequeued));
    // void printCurrentSize(Ptr<QueueDisc> root);    
    // Simulator::Schedule(Seconds(0.1), &printCurrentSize, root);
    Ptr<const Packet> packet;
    Config::ConnectWithoutContext("/NodeList/5/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop",MakeCallback(&TxDropTracer));
    Config::ConnectWithoutContext("/NodeList/5/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",MakeCallback(&RxDropTracer));
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
        std::cout<<"total rxBytes:"<<rxBytes<<"Bytes"<<std::endl;
        std::cout<<"Receive APP"<<i<<" throughput : "<< throughput <<" Mbps"<<std::endl;
        total_thput += throughput;
    }
     std::cout<<"Receive APP TOTAL throughput : "<< total_thput <<" Mbps"<<std::endl;
    std::cout<<"total txdrop Bytes:"<<TxdropByte<<"Bytes"<<std::endl;  
    std::cout<<"total rxdrop Bytes:"<<RxdropByte<<"Bytes"<<std::endl;  
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
