#ifndef FLOWSTATE_H
#define FLOWSTATE_H
#include "ns3/tcp-header.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-base.h"
#include <queue>

namespace ns3{
    class EstimateDelay : public Object{
        public:
        static TypeId GetTypeId (void);
        EstimateDelay();
        double m_estimatedDelay;
        double m_qLong;
        double m_qShort;
        double m_tx;
    };
    class FlowState : public Object {
    public:
        static TypeId GetTypeId (void);
        FlowState();

        uint32_t m_windowSize;
        bool m_isSlowStart;
        SequenceNumber32 m_reduceSeq;
        double m_rtt;
        bool m_canCalcRtt;
        SequenceNumber32 m_calcRttSeqNum;
        Time m_calcRttStartTime;
        Time m_flowStartTime;
        double m_wiredrtt;
        double m_minrtt;
        //Zhuge
        double m_deltaDelay;
        double m_lastTotalDelay;
        std::deque<std::pair<double,double>> m_tokenHistory;
        std::deque<std::pair<double,double>> m_deltaHistory;
        /* 计算ack */
        double m_ackLastSendTime;
        /* 统计RTT预测结果 */
        std::deque<std::pair<RttHistory,EstimateDelay>>      m_flowHistory;
        // OurCC: 记录RTT历史
        std::deque<std::pair<double,std::pair<double,double>>> m_rttHistory;
        std::deque<uint32_t> m_rwndHistory;
        uint16_t m_state;
    };
    
    
}// namespace ns3

#endif // FLOWSTATE_H
