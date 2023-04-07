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

#ifndef FQ_CODEL_QUEUE_DISC
#define FQ_CODEL_QUEUE_DISC

#include "ns3/queue-disc.h"
#include "ns3/object-factory.h"
#include <list>
#include <map>
#include "ns3/ipv4-header.h"
#include "ns3/queue-item.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-queue-disc-item.h"

namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * \brief A flow queue used by the FqCoDel queue disc
 */

class FqFifoFlow : public QueueDiscClass {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief FqCoDelFlow constructor
   */
  FqFifoFlow ();

  virtual ~FqFifoFlow ();

  /**
   * \enum FlowStatus
   * \brief Used to determine the status of this flow queue
   */
  enum FlowStatus
    {
      INACTIVE,
      NEW_FLOW,
      OLD_FLOW
    };

  /**
   * \brief Set the status for this flow
   * \param status the status for this flow
   */
  void SetStatus (FlowStatus status);
  /**
   * \brief Get the status of this flow
   * \return the status of this flow
   */
  FlowStatus GetStatus (void) const;
  /**
   * \brief Set the index for this flow
   * \param index the index for this flow
   */
  void SetIndex (uint64_t index);
  /**
   * \brief Get the index of this flow
   * \return the index of this flow
   */
  uint64_t GetIndex (void) const;

private:
  FlowStatus m_status;  //!< the status of this flow
  uint64_t m_index;     //!< the index for this flow

};


/**
 * \ingroup traffic-control
 *
 * \brief A FqFifo packet queue disc
 */

class FqFifoQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief FqCoDelQueueDisc constructor
   */
  FqFifoQueueDisc ();

  virtual ~FqFifoQueueDisc ();

   /**
    * \brief Set the quantum value.
    *
    * \param quantum The number of bytes each queue gets to dequeue on each round of the scheduling algorithm
    */
   void SetQuantum (uint32_t quantum);

   /**
    * \brief Get the quantum value.
    *
    * \returns The number of bytes each queue gets to dequeue on each round of the scheduling algorithm
    */
   uint32_t GetQuantum (void) const;

  // Reasons for dropping packets
  static constexpr const char* UNCLASSIFIED_DROP = "Unclassified drop";  //!< No packet filter able to classify packet
  static constexpr const char* OVERLIMIT_DROP = "Overlimit drop";        //!< Overlimit dropped packets
  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded


private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  //virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);
  
 

  bool m_useEcn;             //!< True if ECN is used (packets are marked instead of being dropped)


  std::string m_interval;    //!< CoDel interval attribute
  std::string m_target;      //!< CoDel target attribute
  uint32_t m_quantum;        //!< Deficit assigned to flows at each round
  uint32_t m_flows;          //!< Number of flow queues
  uint32_t m_setWays;        //!< size of a set of queues (used by set associative hash)
  uint32_t m_dropBatchSize;  //!< Max number of packets dropped from the fat flow

  std::list<Ptr<FqFifoFlow> > m_flowList;    //!< The list of new flows
  std::list<Ptr<FqFifoFlow> > m_oldFlows;    //!< The list of old flows

  std::map<u_int64_t, uint32_t> m_flowsIndices;    //!< Map with the index of class for each flow
  std::map<uint32_t, uint32_t> m_tags;            //!< Tags used by set associative hash
  std::map<u_int64_t,std::list<double>> m_flowSendTime;
  std::map<u_int64_t,bool> m_canSend;
  

  ObjectFactory m_flowFactory;         //!< Factory to create a new flow
  ObjectFactory m_queueDiscFactory;    //!< Factory to create a new queue
  EventId m_id;                    //!< EventId of the scheduled queue waking event when enough tokens are available
};

} // namespace ns3

#endif /* FQ_CODEL_QUEUE_DISC */

