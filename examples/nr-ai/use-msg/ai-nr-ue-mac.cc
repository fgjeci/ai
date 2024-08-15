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

#define NS_LOG_APPEND_CONTEXT                                            \
  do                                                                     \
    {                                                                    \
      std::clog << " [ C " << GetCellId() << ", b "             \
                << GetBwpId () << ", r " << m_rnti << "] ";           \
    }                                                                    \
  while (false);

#include "ai-nr-ue-mac.h"

#include "ORE-env.h"
#include <ns3/boolean.h>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <iterator>
#include <time.h>

#include <iomanip>
#include <sstream>
#include <string>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AiNrUeMac");
NS_OBJECT_ENSURE_REGISTERED (AiNrUeMac);




TypeId
AiNrUeMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AiNrUeMac")
    .SetParent<NrUeMac> ()
    .AddConstructor<AiNrUeMac> ()
    .AddAttribute ("OReExp",
                   "This specifies if the experiment is a ORe experiment. ",
                   BooleanValue (false), 
                   MakeBooleanAccessor (&AiNrUeMac::m_OReExp),
                   MakeBooleanChecker ())
    .AddAttribute ("PlmnId",
                  "the plmn id identifying the lte coordinator; shall be used to "
                  " distinguish different simulation campaign involving the same xapp",
                  StringValue("111"),
                  MakeStringAccessor(&AiNrUeMac::m_ltePlmnId),
                  MakeStringChecker()
                  )
    .AddAttribute ("REType",
                  "DRE or ORE type ",
                  StringValue("DRE"),
                  MakeStringAccessor(&AiNrUeMac::m_reType),
                  MakeStringChecker()
                  )
    ;

  return tid;
}

AiNrUeMac::AiNrUeMac (void)
{
  NS_LOG_FUNCTION (this);
  // test schedule send Sr after 200 ms
  // Simulator::Schedule(MilliSeconds(200), &AiNrUeMac::SendSlSR, this);
}

void
AiNrUeMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (m_ltePlmnId << m_reType << m_OReExp);

  NrUeMac::DoInitialize();
  m_OReEnv = CreateObject<OREENV> (2333, m_ltePlmnId, m_reType);

}

AiNrUeMac::~AiNrUeMac (void)
{
}

void
AiNrUeMac::DoDispose ()
{
  m_OReEnv = nullptr;
}

std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo>
AiNrUeMac::GetNrSlTxOpportunities (const SfnSf& sfn)
{
  NS_LOG_FUNCTION (sfn << m_ltePlmnId << m_reType << m_OReExp);

  //NR module supported candSsResoA list
  std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> nrCandSsResoA;

  std::list <NrSlCommResourcePool::SlotInfo> candSsResoA;// S_A as per TS 38.214
  uint64_t absSlotIndex = sfn.Normalize ();
  uint8_t bwpId = GetBwpId ();
  uint16_t numerology = sfn.GetNumerology ();

  //Check the validity of the resource selection window configuration (T1 and T2) 
  //and the following parameters: numerology and reservation period.
  uint16_t nsMs = (m_t2-m_t1+1) * (1 / pow(2,numerology)); // number of slots mutiplied by the slot duration in ms
  uint16_t rsvpMs = static_cast <uint16_t> (m_pRsvpTx.GetMilliSeconds ()); // reservation period in ms
  NS_ABORT_MSG_IF (nsMs > rsvpMs, 
      "An error may be generated due to the fact that the resource selection window size is higher than the resource reservation period value. Make sure that (T2-T1+1) x (1/(2^numerology)) < reservation period. Modify the values of T1, T2, numerology, and reservation period accordingly.");

  //step 4 as per TS 38.214 sec 8.1.4
  auto allTxOpps = m_slTxPool->GetNrSlCommOpportunities (absSlotIndex, bwpId, numerology, GetSlActivePoolId (), m_t1, m_t2);
  if (allTxOpps.size () == 0)
    {
      //Since, all the parameters (i.e., T1, T2min, and T2) of the selection
      //window are in terms of physical slots, it may happen that there are no
      //slots available for Sidelink, which depends on the TDD pattern and the
      //Sidelink bitmap.
      return GetNrSupportedList (sfn, allTxOpps);
    }
  candSsResoA = allTxOpps;
  uint32_t mTotal = candSsResoA.size (); // total number of candidate single-slot resources
  int rsrpThrehold = GetSlThresPsschRsrp ();

  if (m_enableSensing)
    {
      if (m_sensingData.size () == 0)
        {
          //no sensing
          nrCandSsResoA = GetNrSupportedList (sfn, candSsResoA);
          return nrCandSsResoA;
        }

      //Copy the buffer so we can trim the buffer as per Tproc0.
      //Note, we do not need to delete the latest measurement
      //from the original buffer because it will be deleted
      //by UpdateSensingWindow method once it is outdated.

      auto sensedData = m_sensingData;

      //latest sensing data is at the end of the list
      //now remove the latest sensing data as per the value of Tproc0. This would
      //keep the size of the buffer equal to [n – T0 , n – Tproc0)

      auto rvIt = sensedData.crbegin ();
      if (rvIt != sensedData.crend ())
        {
          while (sfn.Normalize () - rvIt->sfn.Normalize () <= GetTproc0 ())
            {
              NS_LOG_DEBUG ("IMSI " << m_imsi << " ignoring sensed SCI at sfn " << sfn << " received at " << rvIt->sfn);
              sensedData.pop_back ();
              rvIt = sensedData.crbegin ();
            }
        }

      // calculate all possible transmissions of sensed data
      //using a vector of SlotSensingData, since we need to check all the SCIs
      //and their possible future transmission that are received during the
      //above trimmed sensing window. On each index of the vector, it is a
      //list that holds the info of each received SCI and its possible
      //future transmission.
      std::vector<std::list<SlotSensingData>> allSensingData;
      for (const auto &itSensedSlot:sensedData)
        {
          std::list<SlotSensingData> listFutureSensTx = GetFutSlotsBasedOnSens (itSensedSlot);
          allSensingData.push_back (listFutureSensTx);
        }

      NS_LOG_DEBUG ("Size of allSensingData " << allSensingData.size ());

      //step 5 point 1: We don't need to implement it since we only sense those
      //slots at which this UE does not transmit. This is due to the half
      //duplex nature of the PHY.
      //step 6
      int eliminatedResources = mTotal;
      std::vector<int> srcRnti;// the list of senders of all recieved packets
      std::vector<int> Rc;// the list of Rc values of all recieved packets
      std::vector<int> srcSlotNum;// the list of slots of all recieved packets
      std::vector<int> srcScNum;// the list of subchannels of all recieved packets
      bool firstRun = true;
      // the final step should consider the rssi info during the decision making period
      do
        {
          //following assignment is needed since we might have to perform
          //multiple do-while over the same list by increasing the rsrpThrehold
          if (eliminatedResources > 0)
            {
              eliminatedResources = 0;
            }
          candSsResoA = allTxOpps;
          nrCandSsResoA = GetNrSupportedList (sfn, candSsResoA);
          auto itCandSsResoA = nrCandSsResoA.begin ();
          while (itCandSsResoA != nrCandSsResoA.end ())
            {
              bool erased = false;
              // calculate all proposed transmissions of current candidate resource
              std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> listFutureCands;
              uint16_t pPrimeRsvpTx = m_slTxPool->GetResvPeriodInSlots (GetBwpId (),
                                                                        m_poolId,
                                                                        m_pRsvpTx,
                                                                        m_nrSlUePhySapProvider->GetSlotPeriod ());
              for (uint16_t i = 0; i < m_cResel; i++)
                {
                  auto slAlloc = *itCandSsResoA;
                  slAlloc.sfn.Add (i * pPrimeRsvpTx);
                  listFutureCands.emplace_back (slAlloc);
                }
              // Traverse over all the possible transmissions of each sensed SCI
              for (const auto &itSensedData:allSensingData)
                {
                  // for all proposed transmissions of current candidate resource
                  for (auto &itFutureCand:listFutureCands)
                    {
                      // ore
                      std::vector<double> totRxPower(GetTotalSubCh (m_poolId),0);
                      // ore
                      for (const auto &itFutureSensTx:itSensedData)
                        {
                          if (itFutureCand.sfn.Normalize () == itFutureSensTx.sfn.Normalize ())
                            {
                              uint16_t lastSbChInPlusOne = itFutureSensTx.sbChStart + itFutureSensTx.sbChLength;

                              if(m_OReExp){
                                if(!itFutureSensTx.corrupt)
                                {
                                  if (firstRun)
                                  {
                                    
                                    srcRnti.push_back(itFutureSensTx.srcRnti);
                                    Rc.push_back(itFutureSensTx.Rc);
                                    srcSlotNum.push_back(itFutureCand.sfn.Normalize ()%100);
                                    srcScNum.push_back(itFutureSensTx.sbChStart);
                                  }
                                  totRxPower[itFutureSensTx.sbChStart] += pow(10,itFutureSensTx.slRsrp/10);
                                  if (10*log10(totRxPower[itFutureSensTx.sbChStart]) > rsrpThrehold)
                                    {
                                      itCandSsResoA->occupiedSbCh.insert (itFutureSensTx.sbChStart);
                                      eliminatedResources+=1;//int(itFutureSensTx.sbChLength);
                                    }
                                }
                                else if (itFutureSensTx.corrupt)
                                {
                                  totRxPower[itFutureSensTx.sbChStart] += pow(10,itFutureSensTx.slRsrp/10);
                                  if (10*log10(totRxPower[itFutureSensTx.sbChStart]) > rsrpThrehold)
                                    {
                                      itCandSsResoA->occupiedSbCh.insert (itFutureSensTx.sbChStart);
                                      eliminatedResources+=1;
                                    }
                                }

                              }else{
                                for (uint8_t i = itFutureSensTx.sbChStart; i < lastSbChInPlusOne; i++)
                                {
                                  NS_LOG_DEBUG (this << " Overlapped Slot " << itCandSsResoA->sfn.Normalize () << " occupied " << +itFutureSensTx.sbChLength << " subchannels index " << +itFutureSensTx.sbChStart);
                                  itCandSsResoA->occupiedSbCh.insert (i);
                                }
                                if (itCandSsResoA->occupiedSbCh.size () == GetTotalSubCh (m_poolId))
                                {
                                  if(itFutureSensTx.slRsrp > rsrpThrehold)
                                    {
                                      itCandSsResoA = nrCandSsResoA.erase (itCandSsResoA);
                                      erased = true;
                                      NS_LOG_DEBUG ("Absolute slot number " << itFutureCand.sfn.Normalize () << " erased. Its rsrp : " << itFutureSensTx.slRsrp << " Threshold : " << rsrpThrehold);
                                      //stop traversing over sensing data as we have
                                      //already found the slot to exclude.
                                      break; // break for (const auto &itFutureSensTx:listFutureSensTx)
                                    }
                                }
                              }
                            }
                        }
                      if (erased)
                        {
                          break; // break for (const auto &itFutureCand:listFutureCands)
                        }
                    }
                  if (erased)
                    {
                      break; // break for (const auto &itSensedSlot:allSensingData)
                    }
                }
              if (!erased)
                {
                  itCandSsResoA++;
                }
            }
          //step 7. If the following while will not break, start over do-while
          //loop with rsrpThreshold increased by 3dB
          rsrpThrehold += 3;
          if (rsrpThrehold > 0)
            {
              //0 dBm is the maximum RSRP threshold level so if we reach
              //it, that means all the available slots are overlapping
              //in time and frequency with the sensed slots, and the
              //RSRP of the sensed slots is very high.
              NS_LOG_DEBUG ("Reached maximum RSRP threshold, unable to select resources");
              nrCandSsResoA.erase (nrCandSsResoA.begin (), nrCandSsResoA.end ());
              break; //break do while
            }
        }
      while (nrCandSsResoA.size () < (GetResourcePercentage () / 100.0) * mTotal);

      // ore
      std::ofstream elimResoFile;
      std::ofstream rsrpThreFile;
      if (Simulator::Now ().GetSeconds () > 5 && ((m_imsi >= 25 && m_imsi <= 125) || (m_imsi >= 175 && m_imsi <= 275)))
        {
          elimResoFile.open (m_tracesPath+"eliminatedResources.csv", std::ios_base::app);
          elimResoFile << eliminatedResources << "\n";
          elimResoFile.close();

          rsrpThreFile.open(m_tracesPath+"rsrpThreshold.csv", std::ios_base::app);
          rsrpThreFile << rsrpThrehold - 3 << "\n";
          rsrpThreFile.close();
        }
      if (m_OReExp)
      {
        std::string encodedSrcRnti = "1";
        std::string encodedRc = "1";
        std::string encodedSrcSlot = "1";
        std::string encodedSrcSc = "1";
        for (int i = 0; i < int(srcRnti.size()); ++i)
        {
          if (int(srcRnti[i]) < 10)
          {
            encodedSrcRnti.append("00");
            encodedSrcRnti.append(std::to_string(srcRnti[i]));
          }
          else if (int(srcRnti[i]) < 100)
          {
            encodedSrcRnti.append("0");
            encodedSrcRnti.append(std::to_string(srcRnti[i]));
          }
          else
          {
            encodedSrcRnti.append(std::to_string(srcRnti[i]));
          } 

          if (int(Rc[i]) < 10)
          {
            encodedRc.append("0");
            encodedRc.append(std::to_string(Rc[i]));
          }
          else
          {
            encodedRc.append(std::to_string(Rc[i]));
          }

          if (int(srcSlotNum[i]) < 10)
          {
            encodedSrcSlot.append("0");
            encodedSrcSlot.append(std::to_string(srcSlotNum[i]));
          }
          else
          {
            encodedSrcSlot.append(std::to_string(srcSlotNum[i]));
          }

          encodedSrcSc.append(std::to_string(srcScNum[i]));
        }
        std::ofstream ofs1(m_tracesPath+"encodedSrcRnti.txt", std::ofstream::out);
        ofs1 << encodedSrcRnti <<std::endl;
        ofs1.close();

        std::ofstream ofs2(m_tracesPath+"encodedRc.txt", std::ofstream::out);
        ofs2 << encodedRc <<std::endl;
        ofs2.close();

        std::ofstream ofs3(m_tracesPath+"encodedSrcSlot.txt", std::ofstream::out);
        ofs3 << encodedSrcSlot <<std::endl;
        ofs3.close();

        std::ofstream ofs4(m_tracesPath+"encodedSrcSc.txt", std::ofstream::out);//std::ofstream::trunc);
        ofs4 << encodedSrcSc <<std::endl;
        ofs4.close();

        sleep(.1);

        m_OReEnv->passSensingData(int(m_imsi),double(Simulator::Now ().GetSeconds ()*1000),int(rsrpThrehold),int(eliminatedResources),std::stold(encodedSrcRnti),std::stold(encodedRc),std::stold(encodedSrcSlot),std::stold(encodedSrcSc),1);
      }
      // ore

      NS_LOG_DEBUG (nrCandSsResoA.size () << " slots selected after sensing resource selection from " << mTotal << " slots");
    }
  else
    {
      //no sensing
      nrCandSsResoA = GetNrSupportedList (sfn, candSsResoA);
      NS_LOG_DEBUG ("No sensing: Total slots selected " << nrCandSsResoA.size ());
    }

  return nrCandSsResoA;
}

}