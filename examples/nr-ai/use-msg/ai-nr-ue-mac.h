/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef AI_NR_UE_MAC_H
#define AI_NR_UE_MAC_H

#include <unordered_map>

#include <ns3/nr-ue-mac.h>


#include <ns3/nr-sl-ue-mac-sched-sap.h>


#include "ORE-env.h"

namespace ns3 {


class AiNrUeMac : public NrUeMac
{
  public:
  /**
   * \brief Get the Type id
   * \return the type id
   */
  static TypeId GetTypeId (void);

  /**
   * \brief DoDispose method inherited from Object
   */
  void virtual DoDispose () override;

  /**
   * \brief NrUeMac constructor
   */
  AiNrUeMac (void);
  /**
    * \brief Deconstructor
    */
  ~AiNrUeMac (void) override;
  
  std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> GetNrSlTxOpportunities (const SfnSf& sfn) override;
  
  private:
    Ptr<OREENV> m_OReEnv;
    bool m_OReExp;
};

}


#endif /* NR_UE_MAC_H */
