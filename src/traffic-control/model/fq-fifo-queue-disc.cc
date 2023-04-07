/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
*/

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "fq-fifo-queue-disc.h"
#include "fifo-queue-disc.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FqFifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (FqFifoFlow);

TypeId FqFifoFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqFifoFlow")
    .SetParent<QueueDiscClass> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FqFifoFlow> ()
  ;
  return tid;
}

FqFifoFlow::FqFifoFlow ()
  :
    m_status (INACTIVE),
    m_index (0)
{
  NS_LOG_FUNCTION (this);
}

FqFifoFlow::~FqFifoFlow ()
{
  NS_LOG_FUNCTION (this);
}



void
FqFifoFlow::SetStatus (FlowStatus status)
{
  NS_LOG_FUNCTION (this);
  m_status = status;
}

FqFifoFlow::FlowStatus
FqFifoFlow::GetStatus (void) const
{
  NS_LOG_FUNCTION (this);
  return m_status;
}

void
FqFifoFlow::SetIndex (uint64_t index)
{
  NS_LOG_FUNCTION (this);
  m_index = index;
}

uint64_t
FqFifoFlow::GetIndex (void) const
{
  return m_index;
}


NS_OBJECT_ENSURE_REGISTERED (FqFifoQueueDisc);

TypeId FqFifoQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqFifoQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FqFifoQueueDisc> ()
    .AddAttribute ("UseEcn",
                   "True to use ECN (packets are marked instead of being dropped)",
                   BooleanValue (true),
                   MakeBooleanAccessor (&FqFifoQueueDisc::m_useEcn),
                   MakeBooleanChecker ())
    .AddAttribute ("Interval",
                   "The CoDel algorithm interval for each FQCoDel queue",
                   StringValue ("100ms"),
                   MakeStringAccessor (&FqFifoQueueDisc::m_interval),
                   MakeStringChecker ())
    .AddAttribute ("Target",
                   "The CoDel algorithm target queue delay for each FQCoDel queue",
                   StringValue ("5ms"),
                   MakeStringAccessor (&FqFifoQueueDisc::m_target),
                   MakeStringChecker ())
    .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("10240p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("Flows",
                   "The number of queues into which the incoming packets are classified",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&FqFifoQueueDisc::m_flows),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("DropBatchSize",
                   "The maximum number of packets dropped from the fat flow",
                   UintegerValue (64),
                   MakeUintegerAccessor (&FqFifoQueueDisc::m_dropBatchSize),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

FqFifoQueueDisc::FqFifoQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS),
    m_quantum (0)
{
  NS_LOG_FUNCTION (this);
}

FqFifoQueueDisc::~FqFifoQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
FqFifoQueueDisc::SetQuantum (uint32_t quantum)
{
  NS_LOG_FUNCTION (this << quantum);
  m_quantum = quantum;
}

uint32_t
FqFifoQueueDisc::GetQuantum (void) const
{
  return m_quantum;
}



bool
FqFifoQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  u_int64_t h;
  if (item->GetProtocol() != 2048)
  {
        h=0;
  }
  else{
        TcpHeader tcpHdr;
        item->GetPacket ()->PeekHeader (tcpHdr); 
        Ipv4Header ipv4Hdr = DynamicCast<Ipv4QueueDiscItem>(item)->GetHeader();
        if(item->GetPacket()->GetSize() != 4*tcpHdr.GetLength()) {
            h = 0;
        }
        else {
          //NS_LOG_UNCOND("fqfifoqueue-Disc: enqueue ack packet ("<<item<<") : ("<<item->GetSendTime()<<")");
            uint32_t sip = ipv4Hdr.GetSource().Get();
            uint32_t sport =  tcpHdr.GetSourcePort();
            h = ((uint64_t)sip << 32) | (uint64_t)sport;
        }
        
  }
  Ptr<FqFifoFlow> flow;
  if (m_flowsIndices.find (h) == m_flowsIndices.end ())
  {
    
    flow = m_flowFactory.Create<FqFifoFlow> ();
    Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
    // If CoDel, Set values of CoDelQueueDisc to match this QueueDisc
    Ptr<FifoQueueDisc> fifo = qd->GetObject<FifoQueueDisc> ();
    qd->Initialize ();
    flow->SetQueueDisc (qd);
    flow->SetIndex (h);
    AddQueueDiscClass (flow);

    m_flowsIndices[h] = GetNQueueDiscClasses () - 1;
    m_flowList.push_back(flow);
    if(h!=0) {
      std::list<double> flowSendTime;
      
      m_flowSendTime[h]=flowSendTime;
    }
    m_canSend[flow->GetIndex()] = true;
  }
  else
    {
      flow = StaticCast<FqFifoFlow> (GetQueueDiscClass (m_flowsIndices[h]));
      
    }
  if(h!=0) {
    
    m_flowSendTime[h].push_back(item->GetSendTime());
    //NS_LOG_DEBUG(this<<"--new item("<<item<<","<<item->GetSendTime()<<") into queue No."<<flow->GetIndex()<<" ("<<flow->GetQueueDisc()<<")  listback"<<m_flowSendTime[h].back());

  }
  flow->GetQueueDisc ()->Enqueue (item);

  if (GetCurrentSize () > GetMaxSize ())
    {
      NS_LOG_DEBUG ("Overload; enter FqCodelDrop ()");
      DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
    }

  return true;
}

Ptr<QueueDiscItem>
FqFifoQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<FqFifoFlow> flow;
  Ptr<QueueDiscItem> item;
  Ptr<FqFifoFlow> flow_ptr;
  
  uint32_t flow_num=m_flowList.size();
  u_int32_t tmp_i=0;
  do
    {
      bool found = false;
      
      while (!found && !m_flowList.empty ())
      {

        flow = m_flowList.front ();
        m_flowList.pop_front();
        m_flowList.push_back(flow);
        found=true;
        
        if(m_flowSendTime[flow->GetIndex()].front()<=Simulator::Now().GetSeconds()) {

          m_canSend[flow->GetIndex()] = true;
        }
        else{
          if(m_canSend[flow->GetIndex()]) {
            Time sendTime(m_flowSendTime[flow->GetIndex()].front());
            m_id = Simulator::Schedule (sendTime, &QueueDisc::Run, this);
            m_canSend[flow->GetIndex()] = false;
          }
          
        }
        tmp_i++;
      }
      
      if (!found)
      {
        NS_LOG_DEBUG ("No flow found to dequeue a packet");
        return 0;
      }
      
      if(flow )
      {
        // 不需要推迟的数据流
        if(flow->GetIndex()==0) {
          item = flow->GetQueueDisc ()->Dequeue ();
        }
        else if(m_flowSendTime[flow->GetIndex()].empty()){
          if(tmp_i==flow_num) {
            return 0;
          }
        }
        else if(m_flowSendTime[flow->GetIndex()].front()<=Simulator::Now().GetSeconds())
        {
          
          
          item = flow->GetQueueDisc ()->Dequeue (); 
          //NS_LOG_DEBUG(this<<"--pop item("<<item<<","<<m_flowSendTime[flow->GetIndex()].front()<<") from queue No."<<flow->GetIndex()<<" ("<<flow->GetQueueDisc ()<<") ["<< Simulator::Now().GetSeconds()<<"]");
          m_flowSendTime[flow->GetIndex()].pop_front();
        }
        else{
          
          if(tmp_i==flow_num) {
            return 0;
          }

        }
        
        
      }

      // else if(flow->m_sendTime.front()>Simulator::Now().GetSeconds())
      // {
      //   NS_LOG_DEBUG("item of this flow not ready to dequeue yet, sendTime: "<<flow->m_sendTime.front());
      // }
      
    } while (item == 0);

  

  return item;
}

bool
FqFifoQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("FqFifoQueueDisc cannot have classes");
      return false;
    }

  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("FqFifoQueueDisc cannot have internal queues");
      return false;
    }

  // we are at initialization time. If the user has not set a quantum value,
  // set the quantum to the MTU of the device (if any)
  if (!m_quantum)
    {
      Ptr<NetDeviceQueueInterface> ndqi = GetNetDeviceQueueInterface ();
      Ptr<NetDevice> dev;
      // if the NetDeviceQueueInterface object is aggregated to a
      // NetDevice, get the MTU of such NetDevice
      if (ndqi && (dev = ndqi->GetObject<NetDevice> ()))
        {
          m_quantum = dev->GetMtu ();
          NS_LOG_DEBUG ("Setting the quantum to the MTU of the device: " << m_quantum);
        }

      if (!m_quantum)
        {
          NS_LOG_ERROR ("The quantum parameter cannot be null");
          return false;
        }
    }

  return true;
}

void
FqFifoQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);

  m_flowFactory.SetTypeId ("ns3::FqFifoFlow");

  m_queueDiscFactory.SetTypeId ("ns3::FifoQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (GetMaxSize ()));
  NS_LOG_DEBUG ("InitializeParams finished");
  m_id = EventId ();
}



} // namespace ns3

