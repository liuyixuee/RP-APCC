#ifndef WFQ_QUEUE_DISC_H
#define WFQ_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/object-factory.h"
#include <list>
#include <queue>
#include <map>

namespace ns3 {

class WFQClass : public QueueDiscClass
{
public:

    static TypeId GetTypeId (void);
    WFQClass ();
    virtual ~WFQClass();

    Ptr<QueueDisc> qdisc;

    double_t m_lastFinTime;
    double_t m_weight;

	uint32_t m_packetsInWFQQueue;
	uint32_t m_maxPackets;
    bool m_active;
    
};


class WFQQueueItem : public Object{
public:
    static TypeId GetTypeId (void);
    WFQQueueItem ();
    virtual ~WFQQueueItem();
	Ptr<QueueDiscItem> m_packet;
	double_t m_finishTime;
	uint16_t m_classIndex;
};

struct CompareFinishTime {
    bool operator()(Ptr<WFQQueueItem> qi1, Ptr<WFQQueueItem> qi2) {
        return qi1->m_finishTime > qi2->m_finishTime;
    }
};

class WFQQueueDisc : public QueueDisc
{
public:

    static TypeId GetTypeId (void);
    WFQQueueDisc ();
    virtual ~WFQQueueDisc ();

    void AddWFQClasses (void);
   

private:
    
    virtual bool DoEnqueue (Ptr<QueueDiscItem> p);
    virtual Ptr<QueueDiscItem> DoDequeue (void);
    virtual Ptr<const QueueDiscItem> DoPeek (void) const;
    uint16_t Classify(Ptr<QueueDiscItem> p );
    virtual bool CheckConfig (void);
    virtual void InitializeParams (void);

    // calculates the packet's virtual finish time:
	// finish_time(packet i) = max(finish_time(i-1) , virtual_time(arrival_time(i))) + (packet_size_in_bits / weight)
	double_t CalculateFinishTime(double_t startTime, uint32_t packetSize, double_t weight);
    double_t CalculateActiveSum(void);
	int16_t GetMinActiveClass(void);

    double_t m_inputFirstWeight;
    double_t m_inputSecondWeight;
    uint32_t m_secondQueuePort;
    double_t m_linkCapacity;
    bool m_isInitialized;

    uint32_t n_classifies = 0;
    double_t m_virtualTime;
    double_t v_chk;
	double_t t_chk;
    

    ObjectFactory m_classFactory;         //!< Factory to create a new class
    ObjectFactory m_queueDiscFactory; //!< Factory to create a new queue
    std::vector<Ptr<WFQClass> > m_WFQs;
    std::priority_queue<Ptr<WFQQueueItem>, std::vector<Ptr<WFQQueueItem>>, CompareFinishTime> m_wfqPQ;
    std::map<uint64_t, double_t> getFinishTimeCollection() const;
    //std::map<uint32_t, uint64_t> m_virtualTime;

};

} // namespace ns3

#endif