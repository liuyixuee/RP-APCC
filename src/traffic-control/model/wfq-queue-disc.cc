#include "ns3/log.h"
#include "wfq-queue-disc.h"
#include "ns3/queue.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/ppp-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-header.h"
#include <algorithm>
#include <float.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WFQQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (WFQClass);

TypeId
WFQClass::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::WFQClass")
      .SetParent<QueueDiscClass> ()
      .SetGroupName ("TrafficControl")
      .AddConstructor<WFQClass> ()
    ;
    return tid;
}

WFQClass::WFQClass ()
{
    NS_LOG_FUNCTION (this);
	m_packetsInWFQQueue = 0;
	m_lastFinTime = 0;
	m_active = false;
    m_maxPackets = 1000;

}

WFQClass::~WFQClass ()
{
    NS_LOG_FUNCTION (this);
}

NS_OBJECT_ENSURE_REGISTERED (WFQQueueItem);

TypeId
WFQQueueItem::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::WFQQueueItem")
      .SetParent<Object> ()
      .SetGroupName ("TrafficControl")
      .AddConstructor<WFQQueueItem> ()
    ;
    return tid;
}

WFQQueueItem::WFQQueueItem ()
{
    NS_LOG_FUNCTION (this);
	
}

WFQQueueItem::~WFQQueueItem ()
{
    NS_LOG_FUNCTION (this);
}

NS_OBJECT_ENSURE_REGISTERED (WFQQueueDisc);

TypeId
WFQQueueDisc::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::WFQQueueDisc")
      .SetParent<QueueDisc> ()
      .SetGroupName ("TrafficControl")
      .AddConstructor<WFQQueueDisc> ()
      .AddAttribute("FirstWeight",
	        		"The first queue's weight",
					DoubleValue(0),
					MakeDoubleAccessor(&WFQQueueDisc::m_inputFirstWeight),
					MakeDoubleChecker<double_t>())

	  .AddAttribute("SecondWeight",
					"The second queue's weight",
					DoubleValue(0),
					MakeDoubleAccessor(&WFQQueueDisc::m_inputSecondWeight),
					MakeDoubleChecker<double_t>())

	  .AddAttribute("LinkCapacity",
					"Link Capacity",
					DoubleValue(0),
					MakeDoubleAccessor(&WFQQueueDisc::m_linkCapacity),
					MakeDoubleChecker<double_t>())
      .AddAttribute("SecondQueuePort",
					"The destination port number for second queue traffic.",
					UintegerValue(10),
					MakeUintegerAccessor(&WFQQueueDisc::m_secondQueuePort),
					MakeUintegerChecker<uint32_t>())
    ;
    return tid;
}

WFQQueueDisc::WFQQueueDisc ()
{
    NS_LOG_FUNCTION (this);
	NS_LOG_INFO("\tTworze Nowa QUEUE DISC");
	m_isInitialized = false;
	m_virtualTime = 0.0;
	v_chk = 0.0;
	t_chk = 0.0;
	
}

WFQQueueDisc::~WFQQueueDisc ()
{
    NS_LOG_FUNCTION (this);
	NS_LOG_INFO("\tNiszcze QUEUE DISC");
}

void
WFQQueueDisc::AddWFQClasses (void)
{
    Ptr<WFQClass> wfqClass1 = m_classFactory.Create<WFQClass> ();
    Ptr<WFQClass> wfqClass2 = m_classFactory.Create<WFQClass> ();
    wfqClass1->m_weight = m_inputFirstWeight;
    wfqClass2->m_weight = m_inputSecondWeight;

    m_WFQs.push_back(wfqClass1);
    m_WFQs.push_back(wfqClass2);

}

uint16_t WFQQueueDisc::Classify(Ptr<QueueDiscItem> p) {
	
	NS_LOG_FUNCTION(this << p);
	Ptr<Ipv4QueueDiscItem> ipv4Item = DynamicCast<Ipv4QueueDiscItem> (p);
    Ipv4Header hdr = ipv4Item->GetHeader ();
    //Ipv4Address src = hdr.GetSource ();
    //Ipv4Address dest = hdr.GetDestination ();
    uint8_t prot = hdr.GetProtocol ();
    uint16_t fragOffset = hdr.GetFragmentOffset ();

    TcpHeader tcpHdr;
    UdpHeader udpHdr;
    //uint16_t srcPort = 0;
    uint16_t destPort = 0;

	uint16_t classIndex;
	Ptr<Packet> pkt = ipv4Item->GetPacket ();
	n_classifies++;
	NS_LOG_INFO("\tNumber of classifies: "<< n_classifies);
	if (prot == 17 && fragOffset == 0) {
		
		pkt->PeekHeader (udpHdr);
        //srcPort = tcpHdr.GetSourcePort ();
        destPort = udpHdr.GetDestinationPort ();

		if (destPort == m_secondQueuePort) {
			NS_LOG_INFO("\tclassifier: second queue udp");
			classIndex = 1;
		} else {
			NS_LOG_INFO("\tclassifier: first queue udp");
			classIndex = 0;
		}
	}

	else if (prot == 6 && fragOffset == 0) {

		pkt->PeekHeader (tcpHdr);
        //srcPort = tcpHdr.GetSourcePort ();
        destPort = tcpHdr.GetDestinationPort ();

		if (destPort == m_secondQueuePort) {
			NS_LOG_INFO("\tclassifier: second queue tcp");
			classIndex = 1;
		} else {
			NS_LOG_INFO("\tclassifier: first queue tcp");
			classIndex = 0;
		}

	} else {
		NS_LOG_INFO("\tclassifier: unrecognized transport protocol");
		classIndex = 0;
	}


	return classIndex;
}


bool
WFQQueueDisc::DoEnqueue (Ptr<QueueDiscItem> p)
{
    NS_LOG_FUNCTION(this << p);
	double_t packetFinishTime, packetStartTime;

	uint16_t classIndex = Classify(p);

	
	if (m_isInitialized == false)
	{
		NS_LOG_INFO("\tHALO:");
		AddWFQClasses();
		m_isInitialized = true;
	}
	

	if(m_WFQs[classIndex]->GetQueueDisc() == 0){
		NS_LOG_INFO("\tTEST:" );
		Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
    	qd->Initialize ();
		m_WFQs[classIndex]->SetQueueDisc (qd);
    	AddQueueDiscClass (m_WFQs[classIndex]);
		
	}


	if (m_WFQs[classIndex]->m_packetsInWFQQueue >= m_WFQs[classIndex]->m_maxPackets) {
		NS_LOG_INFO("Queue full: dropping packet");
		DropBeforeEnqueue (p, "Queue disc limit exceeded");
		return false;
	}



	// updating virtual time based on iterated deletion
	bool fin = false;
	int16_t minActiveClassIndex;
	double_t minActiveLastFinishTime;
	double_t delta;
	double_t t_now = Simulator::Now().GetSeconds();
	
	while (!fin)
	{
		delta = t_now - t_chk;
		minActiveClassIndex = GetMinActiveClass();
		if (minActiveClassIndex == -1)
			fin = true;
		else
		{
			minActiveLastFinishTime = m_WFQs[minActiveClassIndex]->m_lastFinTime;
			if (!(minActiveLastFinishTime <= (v_chk + delta / CalculateActiveSum())))
				fin = true;
		}

		if (fin == false)
		{
			t_chk = t_chk + (minActiveLastFinishTime - v_chk) * CalculateActiveSum();
			v_chk = minActiveLastFinishTime;
			m_WFQs[minActiveClassIndex]->m_active = false;
		}

		else
		{
			if (minActiveClassIndex == -1)
			{
				m_virtualTime = v_chk; // = 0;
				
			}
			else{
			m_virtualTime = v_chk + delta / CalculateActiveSum();
			v_chk = m_virtualTime;
			t_chk = t_now;
			}
			
		}
	}

	m_WFQs[classIndex]->m_active = true;

	packetStartTime = m_WFQs[classIndex]->m_lastFinTime > m_virtualTime ? m_WFQs[classIndex]->m_lastFinTime : m_virtualTime;
	packetFinishTime = CalculateFinishTime(packetStartTime, p->GetSize(), m_WFQs[classIndex]->m_weight);

	m_WFQs[classIndex]->m_lastFinTime = packetFinishTime;

	Ptr<WFQQueueItem> queueItem1 = CreateObject<WFQQueueItem>();
	queueItem1->m_packet = p;
	queueItem1->m_finishTime = packetFinishTime;
	queueItem1->m_classIndex = classIndex;
	m_wfqPQ.push(queueItem1);
	NS_LOG_INFO("Size of m_wfqPQ: " << m_wfqPQ.size());
	NS_LOG_INFO("Virtual fin time when enqueue: " << queueItem1->m_finishTime);
	bool retval = m_WFQs[classIndex]->GetQueueDisc ()->Enqueue (p);
	NS_LOG_INFO("REAL TIME Timestamp:: " << queueItem1->m_packet->GetTimeStamp());
	m_WFQs[classIndex]->m_packetsInWFQQueue++;
	NS_LOG_INFO("Number of packets " << m_WFQs[classIndex]->GetQueueDisc()->GetNPackets());

	return retval;
}

Ptr<QueueDiscItem>
WFQQueueDisc::DoDequeue (void)
{
    NS_LOG_FUNCTION (this);

    uint16_t minClassIndex;
	Ptr<QueueDiscItem> p;

	if (!m_wfqPQ.empty())
	{		
		
		Ptr<WFQQueueItem> queueItem = m_wfqPQ.top();
		minClassIndex = queueItem->m_classIndex;
		NS_LOG_INFO("Virtual fin time when dequeue: " << queueItem->m_finishTime);
		//p = queueItem->m_packet;
		m_wfqPQ.pop();
		m_WFQs[minClassIndex]->m_packetsInWFQQueue--;
		p = m_WFQs[minClassIndex]->GetQueueDisc ()->Dequeue ();
		NS_LOG_INFO("REAL TIME Timestamp:: " << queueItem->m_packet->GetTimeStamp());
		NS_LOG_INFO("Dequeued packet " << p-> GetPacket());
		NS_LOG_INFO("Number of packets " << m_WFQs[minClassIndex]->m_packetsInWFQQueue);
		
		return p;
	}else
	{
		NS_LOG_INFO("all queues empty");
		return 0;
	}
	
}

Ptr<const QueueDiscItem>
WFQQueueDisc::DoPeek (void) const
{
    NS_LOG_FUNCTION (this);
	return 0;

}

double_t WFQQueueDisc::CalculateFinishTime (double_t startTime, uint32_t packetSize, double_t weight) {

	return startTime + (8 * packetSize / (weight * m_linkCapacity));
}

int16_t WFQQueueDisc::GetMinActiveClass(void) {

	double_t minLastFinishTime = DBL_MAX;
	int16_t minActiveClassIndex = -1;

	for (unsigned int i=0; i<m_WFQs.size(); i++) {

		if (m_WFQs[i]->m_active == true)
			if (m_WFQs[i]->m_lastFinTime < minLastFinishTime)
			{
				minLastFinishTime = m_WFQs[i]->m_lastFinTime;
				minActiveClassIndex = i;
			}
	}

	return minActiveClassIndex;
}

double_t WFQQueueDisc::CalculateActiveSum(void) {
	double_t weightSum = 0;

	for (unsigned int i=0; i<m_WFQs.size(); i++) {

		if (m_WFQs[i]->m_active == true)
			weightSum += m_WFQs[i]->m_weight;
	}

	return weightSum;
}

bool
WFQQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  /*
  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("WFQQueueDisc cannot have packet filter");
      return false;
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("WFQQueueDisc needs 1 internal queue");
      return false;
    }
*/
  return true;
}

void
WFQQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);

  m_classFactory.SetTypeId ("ns3::WFQClass");

  m_queueDiscFactory.SetTypeId ("ns3::FifoQueueDisc");

}

}