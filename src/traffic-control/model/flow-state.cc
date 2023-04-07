
#include "ns3/tcp-header.h"
#include "flow-state.h"

/******************
 * FlowState
 *****************/
namespace ns3{
    TypeId FlowState::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::FlowState")
            .SetParent<Object> ()
            ;
        return tid;
    }

    FlowState::FlowState(){
        m_windowSize = 10 * 1448;
        m_isSlowStart = true;
        m_reduceSeq = SequenceNumber32(0);
        m_rtt = -1.0;
        m_canCalcRtt = true;
        m_calcRttSeqNum = SequenceNumber32(0);
        m_calcRttStartTime = Time::Min ();
        m_wiredrtt = 0.0;
        m_minrtt = 0.0;

        m_deltaDelay = 0.0;
        m_lastTotalDelay = 0.0;
        /* 计算ack */
        m_ackLastSendTime = 0.0;
        m_state = 0;
    }
    TypeId EstimateDelay::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::EstimateDelay")
            .SetParent<Object> ()
            ;
        return tid;
    }
    EstimateDelay::EstimateDelay(){
        m_estimatedDelay = 0;
        m_qLong = 0;
        m_qShort = 0;
        m_tx = 0;
    }
}// namespace ns3


