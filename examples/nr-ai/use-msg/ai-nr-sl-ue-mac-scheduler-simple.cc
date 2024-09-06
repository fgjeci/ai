﻿/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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

#include "ai-nr-sl-ue-mac-scheduler-simple.h"

#include <ns3/log.h>
#include <algorithm>

#include <ns3/boolean.h>
#include <string>
#include <iostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AiNrSlUeMacSchedulerSimple");
NS_OBJECT_ENSURE_REGISTERED (AiNrSlUeMacSchedulerSimple);

AiNrSlUeMacSchedulerSimple::AiNrSlUeMacSchedulerSimple ()
{
  NS_LOG_FUNCTION (this);
}

void
AiNrSlUeMacSchedulerSimple::DoInitialize (void)
{
  NS_LOG_FUNCTION(m_OReExp << m_ltePlmnId << m_reType);
  NrSlUeMacSchedulerNs3::DoInitialize();
  m_OReEnv = CreateObject<OREENV> (2333, m_ltePlmnId, m_reType);  
}

AiNrSlUeMacSchedulerSimple::~AiNrSlUeMacSchedulerSimple()
{
  m_OReEnv = nullptr;
}

TypeId
AiNrSlUeMacSchedulerSimple::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AiNrSlUeMacSchedulerSimple")
    .SetParent<NrSlUeMacSchedulerNs3> ()
    .AddConstructor<AiNrSlUeMacSchedulerSimple> ()
    .SetGroupName ("nr")
    .AddAttribute ("OReExp",
                   "This specifies if the experiment is a ORe experiment. ",
                   BooleanValue (false), 
                   MakeBooleanAccessor (&AiNrSlUeMacSchedulerSimple::m_OReExp),
                   MakeBooleanChecker ())
    .AddAttribute ("TracesPath",
                   "The path where to store the path. ",
                   StringValue ("./"),
                   MakeStringAccessor (&AiNrSlUeMacSchedulerSimple::m_tracesPath),
                   MakeStringChecker ())
    .AddAttribute ("PlmnId",
                  "the plmn id identifying the lte coordinator; shall be used to "
                  " distinguish different simulation campaign involving the same xapp",
                  StringValue("111"),
                  MakeStringAccessor(&AiNrSlUeMacSchedulerSimple::m_ltePlmnId),
                  MakeStringChecker()
                  )
    .AddAttribute ("REType",
                  "DRE or ORE type ",
                  StringValue("DRE"),
                  MakeStringAccessor(&AiNrSlUeMacSchedulerSimple::m_reType),
                  MakeStringChecker()
                  )
  ;
  return tid;
}

bool
AiNrSlUeMacSchedulerSimple::DoNrSlAllocation (const std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo>& txOpps,
                                            const std::shared_ptr<NrSlUeMacSchedulerDstInfo> &dstInfo,
                                            std::set<NrSlSlotAlloc> &slotAllocList)
{
  NS_LOG_FUNCTION (txOpps.size () << dstInfo->GetNrSlLCG ().size());
  bool allocated = false;
  NS_ASSERT_MSG (txOpps.size () > 0, "Scheduler received an empty txOpps list from UE MAC");
  const auto & lcgMap = dstInfo->GetNrSlLCG (); //Map of unique_ptr should not copy

  NS_ASSERT_MSG (lcgMap.size () == 1, "AiNrSlUeMacSchedulerSimple can handle only one LCG");

  std::vector<uint8_t> lcVector = lcgMap.begin ()->second->GetLCId ();
  NS_ASSERT_MSG (lcVector.size () == 1, "AiNrSlUeMacSchedulerSimple can handle only one LC");

  uint32_t bufferSize = lcgMap.begin ()->second->GetTotalSizeOfLC (lcVector.at (0));

  NS_LOG_DEBUG("Buffer size " << bufferSize);
  if (m_OReExp)
  {
    double encodedSelectionInstructions = m_OReEnv->getResourceSelections(0);
    encodedSelectionInstructions++;//dummy for the moment
  }

  if (bufferSize == 0)
    {
      return allocated;
    }

  NS_ASSERT_MSG (IsNrSlMcsFixed (), "Attribute FixNrSlMcs must be true for AiNrSlUeMacSchedulerSimple scheduler");

  // ore
  if (m_OReExp)
  {
    
    // double encodedSelectionInstructions = m_OReEnv->getResourceSelections(0);
    // encodedSelectionInstructions++;//dummy for the moment
    std::ifstream f(m_tracesPath + "encodedSelectionInstructions.txt");
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string encodedSelectionInstructionsString = ss.str();


    // std::cout<<"nr-sl-ue-mac-scheduler-simple"<<std::endl;
    NS_LOG_INFO("Encoded selec string" << encodedSelectionInstructionsString);
    int selectionMode = std::stoi(encodedSelectionInstructionsString.substr(0,1));
    encodedSelectionInstructionsString.erase(0,1);

    if (selectionMode == 1)//O-Re mode, overwritable resources found
    {
      NS_LOG_DEBUG("ORE mode");
      std::vector<uint8_t> chosenSlots;
      std::vector<uint8_t> chosenSubChannels;
      uint8_t totalTx = std::stoi(encodedSelectionInstructionsString.substr(0,1));
      encodedSelectionInstructionsString.erase(0,1);


      while(encodedSelectionInstructionsString.length() > 0)
      {
        chosenSlots.push_back((uint8_t) std::stoi(encodedSelectionInstructionsString.substr(0,2)));
        encodedSelectionInstructionsString.erase(0,2);
        chosenSubChannels.push_back((uint8_t) std::stoi(encodedSelectionInstructionsString.substr(0,1)));
        encodedSelectionInstructionsString.erase(0,1);
      }

      NS_LOG_DEBUG("Chosen slots " << chosenSlots.size() << " first " << chosenSlots[0]);
      NS_LOG_DEBUG("Chosen chosenSubChannels " << chosenSubChannels.size() << " first " << chosenSubChannels[0]);
      NS_LOG_DEBUG("Tx Pps size " << txOpps.size () << " totalTx " << totalTx);

      std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> selectedTxOpps;

      //RandomlySelectSlots start
      std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> newTxOpps;

      if (txOpps.size () > totalTx)
        {
          if (totalTx % 2 == 0)
            {
              while (newTxOpps.size () != totalTx)
                {
                  auto txOppsIt = txOpps.begin ();
                  // Walk through list until the random element is reached
                  std::advance (txOppsIt, chosenSlots[0]);
                  //copy the randomly selected slot info into the new list if it has at least one free subchannel
                  newTxOpps.emplace_back (*txOppsIt);
                  //erase the selected one from the list, if the itterator is full we dont want to try to use it again
                  chosenSlots.erase(chosenSlots.begin());
                }
            }
          else
            {
              while (newTxOpps.size () != totalTx)
                {
                  if (chosenSlots.size () > 0)
                  {
                    auto txOppsIt = txOpps.begin ();
                    // Walk through list until the random element is reached
                    std::advance (txOppsIt, chosenSlots[0]);
                    //copy the randomly selected slot info into the new list if it has at least one free subchannel
                    newTxOpps.emplace_back (*txOppsIt);
                    //erase the selected one from the list, if the itterator is full we dont want to try to use it again
                    chosenSlots.erase(chosenSlots.begin());
                  }
                  else//we have to select one randomly
                  {
                    while (newTxOpps.size () != totalTx)
                    {
                        auto txOppsIt = txOpps.begin ();
                        // Walk through list until the random element is reached
                        std::advance (txOppsIt, m_uniformVariable->GetInteger (0, txOpps.size () - 1));
                        //copy the randomly selected slot info into the new list if it has at least one free subchannel
                        if (txOppsIt->occupiedSbCh.size() < GetTotalSubCh() || txOpps.size () <= totalTx)
                        {
                          newTxOpps.emplace_back (*txOppsIt);
                          bool scNotChosen = true;
                          while (scNotChosen)
                          {
                            int randSc = m_uniformVariable->GetInteger (0, GetTotalSubCh() - 1);
                            if (!std::count(txOppsIt->occupiedSbCh.begin(), txOppsIt->occupiedSbCh.end(), randSc))
                            {
                              scNotChosen = false;
                              chosenSubChannels.emplace_back((uint8_t) randSc);
                            }
                          }
                        }
                      }
                    }
                }
            }
        }
      else
        {
          newTxOpps = txOpps;
        }
      //sort the list by SfnSf before returning. This has to be done in a custom way because we have to sort both newTxOpps and chosenSubChannels at the same time in the same way.

      std::vector <NrSlUeMacSchedSapProvider::NrSlSlotInfo> newTxOppsVec{newTxOpps.begin(),newTxOpps.end()};

      
      std::vector <NrSlUeMacSchedSapProvider::NrSlSlotInfo> sortedNewTxOppsVec;
      std::vector<uint8_t> sortedChosenSubChannels;


      std::vector <std::pair<NrSlUeMacSchedSapProvider::NrSlSlotInfo, uint8_t>> PRBs;
      for (int i = 0; i < int(newTxOppsVec.size()); ++i)
      {
        PRBs.push_back(std::pair<NrSlUeMacSchedSapProvider::NrSlSlotInfo&, uint8_t&>(newTxOppsVec[i],chosenSubChannels[i]));
      }

      sort(PRBs.begin(), PRBs.end());

      for (int i = 0; i < int(PRBs.size()); ++i)
      {
        sortedNewTxOppsVec.push_back(PRBs[i].first);
        sortedChosenSubChannels.push_back(PRBs[i].second);
      }

      std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> sortedNewTxOpps(sortedNewTxOppsVec.begin(), sortedNewTxOppsVec.end());

      newTxOpps.sort ();

      selectedTxOpps =  sortedNewTxOpps;
      NS_LOG_DEBUG("selectedTxOpps " << selectedTxOpps.size());
      //RandomlySelectSlots end

      NS_ASSERT_MSG (selectedTxOpps.size () > 0, "Scheduler should select at least 1 slot from txOpps");
      uint32_t tbs = 0;
      uint8_t assignedSbCh = 0;
      uint16_t availableSymbols = selectedTxOpps.begin ()->slPsschSymLength;
      uint16_t sbChSize = selectedTxOpps.begin ()->slSubchannelSize;
      NS_LOG_DEBUG ("Total available symbols for PSSCH = " << availableSymbols);
      NS_LOG_DEBUG ("sbChSize " << sbChSize);
      //find the minimum available number of contiguous sub-channels in the
      //selected TxOpps
      auto sbChInfo = GetAvailSbChInfo (selectedTxOpps);
      NS_ABORT_MSG_IF (sbChInfo.availSbChIndPerSlot.size () != selectedTxOpps.size (), "subChInfo vector does not have info for all the selected slots");
      do
        {
          assignedSbCh++;
          tbs = GetNrSlAmc ()->CalculateTbSize (dstInfo->GetDstMcs (), sbChSize * assignedSbCh * availableSymbols);
        }
      while (tbs < bufferSize + 5 /*(5 bytes overhead of SCI format 2A)*/ && (sbChInfo.numSubCh - assignedSbCh) > 0);
      tbs = tbs - 5 /*(5 bytes overhead of SCI stage 2)*/;


      std::vector <uint8_t> startSubChIndexPerSlot = sortedChosenSubChannels;
      //RandSelSbChStart start
      
      //RandSelSbChStart end

      allocated = true;

      auto itsbChIndexPerSlot = startSubChIndexPerSlot.cbegin ();
      auto itTxOpps = selectedTxOpps.cbegin ();
      for (; itTxOpps != selectedTxOpps.cend () && itsbChIndexPerSlot != startSubChIndexPerSlot.cend (); ++itTxOpps, ++itsbChIndexPerSlot)
        {
          NrSlSlotAlloc slotAlloc;
          slotAlloc.sfn = itTxOpps->sfn;
          slotAlloc.dstL2Id = dstInfo->GetDstL2Id ();
          slotAlloc.priority = lcgMap.begin ()->second->GetLcPriority (lcVector.at (0));
          SlRlcPduInfo slRlcPduInfo (lcVector.at (0), tbs);
          slotAlloc.slRlcPduInfo.push_back (slRlcPduInfo);
          slotAlloc.mcs = dstInfo->GetDstMcs ();
          //PSCCH
          slotAlloc.numSlPscchRbs = itTxOpps->numSlPscchRbs;
          slotAlloc.slPscchSymStart = itTxOpps->slPscchSymStart;
          slotAlloc.slPscchSymLength = itTxOpps->slPscchSymLength;
          //PSSCH
          slotAlloc.slPsschSymStart = itTxOpps->slPsschSymStart;
          slotAlloc.slPsschSymLength = availableSymbols;
          slotAlloc.slPsschSubChStart = *itsbChIndexPerSlot;
          slotAlloc.slPsschSubChLength = assignedSbCh;
          slotAlloc.maxNumPerReserve = itTxOpps->slMaxNumPerReserve;
          slotAlloc.ndi = slotAllocList.empty () == true ? 1 : 0;
          slotAlloc.rv = GetRv (static_cast<uint8_t>(slotAllocList.size ()));
          if (static_cast<uint16_t>(slotAllocList.size ()) % itTxOpps->slMaxNumPerReserve == 0)
            {
              slotAlloc.txSci1A = true;
              if (slotAllocList.size () + itTxOpps->slMaxNumPerReserve <= selectedTxOpps.size ())
                {
                  slotAlloc.slotNumInd = itTxOpps->slMaxNumPerReserve;
                }
              else
                {
                  slotAlloc.slotNumInd = selectedTxOpps.size () - slotAllocList.size ();
                }
            }
          else
            {
              slotAlloc.txSci1A = false;
              //Slot, which does not carry SCI 1-A can not indicate future TXs
              slotAlloc.slotNumInd = 0;
            }

          slotAllocList.emplace (slotAlloc);
        }
      lcgMap.begin ()->second->AssignedData (lcVector.at (0), tbs);
      return allocated;


      
    }
    else//D-Re mode, no (or not enough) overwritable resources found
    {
      NS_LOG_DEBUG("ODE mode");
      int NSe = std::stoi(encodedSelectionInstructionsString.substr(0,1));
      NS_LOG_DEBUG("NSe " << NSe);
      NS_LOG_DEBUG("Tx Pps size " << txOpps.size ());

      std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> selectedTxOpps;
      selectedTxOpps = RandomlySelectSlots (txOpps,uint8_t(NSe));
      NS_ASSERT_MSG (selectedTxOpps.size () > 0, "Scheduler should select at least 1 slot from txOpps");
      uint32_t tbs = 0;
      uint8_t assignedSbCh = 0;
      uint16_t availableSymbols = selectedTxOpps.begin ()->slPsschSymLength;
      uint16_t sbChSize = selectedTxOpps.begin ()->slSubchannelSize;
      NS_LOG_DEBUG ("Total available symbols for PSSCH = " << availableSymbols);
      //find the minimum available number of contiguous sub-channels in the
      //selected TxOpps
      NS_LOG_DEBUG ("sbChSize " << sbChSize);
      NS_LOG_DEBUG("selectedTxOpps size " << selectedTxOpps.size());
      auto sbChInfo = GetAvailSbChInfo (selectedTxOpps);

      NS_ABORT_MSG_IF (sbChInfo.availSbChIndPerSlot.size () != selectedTxOpps.size (), "subChInfo vector does not have info for all the selected slots");
      do
        {
          assignedSbCh++;
          tbs = GetNrSlAmc ()->CalculateTbSize (dstInfo->GetDstMcs (), sbChSize * assignedSbCh * availableSymbols);
        }
      while (tbs < bufferSize + 5 /*(5 bytes overhead of SCI format 2A)*/ && (sbChInfo.numSubCh - assignedSbCh) > 0);

      //Now, before allocating bytes to LCs we subtract 5 bytes for SCI format 2A
      //since we already took it into account while computing the TB size.
      tbs = tbs - 5 /*(5 bytes overhead of SCI stage 2)*/;

      //NS_LOG_DEBUG ("number of allocated subchannles = " << +assignedSbCh);

      std::vector <uint8_t> startSubChIndexPerSlot = RandSelSbChStart (sbChInfo, assignedSbCh);

      allocated = true;

      auto itsbChIndexPerSlot = startSubChIndexPerSlot.cbegin ();
      auto itTxOpps = selectedTxOpps.cbegin ();

      for (; itTxOpps != selectedTxOpps.cend () && itsbChIndexPerSlot != startSubChIndexPerSlot.cend (); ++itTxOpps, ++itsbChIndexPerSlot)
        {
          NrSlSlotAlloc slotAlloc;
          slotAlloc.sfn = itTxOpps->sfn;
          slotAlloc.dstL2Id = dstInfo->GetDstL2Id ();
          slotAlloc.priority = lcgMap.begin ()->second->GetLcPriority (lcVector.at (0));
          SlRlcPduInfo slRlcPduInfo (lcVector.at (0), tbs);
          slotAlloc.slRlcPduInfo.push_back (slRlcPduInfo);
          slotAlloc.mcs = dstInfo->GetDstMcs ();
          //PSCCH
          slotAlloc.numSlPscchRbs = itTxOpps->numSlPscchRbs;
          slotAlloc.slPscchSymStart = itTxOpps->slPscchSymStart;
          slotAlloc.slPscchSymLength = itTxOpps->slPscchSymLength;
          //PSSCH
          slotAlloc.slPsschSymStart = itTxOpps->slPsschSymStart;
          slotAlloc.slPsschSymLength = availableSymbols;
          slotAlloc.slPsschSubChStart = *itsbChIndexPerSlot;
          slotAlloc.slPsschSubChLength = assignedSbCh;
          slotAlloc.maxNumPerReserve = itTxOpps->slMaxNumPerReserve;
          slotAlloc.ndi = slotAllocList.empty () == true ? 1 : 0;
          slotAlloc.rv = GetRv (static_cast<uint8_t>(slotAllocList.size ()));
          if (static_cast<uint16_t>(slotAllocList.size ()) % itTxOpps->slMaxNumPerReserve == 0)
            {
              slotAlloc.txSci1A = true;
              if (slotAllocList.size () + itTxOpps->slMaxNumPerReserve <= selectedTxOpps.size ())
                {
                  slotAlloc.slotNumInd = itTxOpps->slMaxNumPerReserve;
                }
              else
                {
                  slotAlloc.slotNumInd = selectedTxOpps.size () - slotAllocList.size ();
                }
            }
          else
            {
              slotAlloc.txSci1A = false;
              //Slot, which does not carry SCI 1-A can not indicate future TXs
              slotAlloc.slotNumInd = 0;
            }

          slotAllocList.emplace (slotAlloc);
        }
      
      lcgMap.begin ()->second->AssignedData (lcVector.at (0), tbs);
      return allocated;

    }
  }
  else{
  // ore

    std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> selectedTxOpps;
    selectedTxOpps = RandomlySelectSlots (txOpps);
    NS_ASSERT_MSG (selectedTxOpps.size () > 0, "Scheduler should select at least 1 slot from txOpps");
    uint32_t tbs = 0;
    uint8_t assignedSbCh = 0;
    uint16_t availableSymbols = selectedTxOpps.begin ()->slPsschSymLength;
    uint16_t sbChSize = selectedTxOpps.begin ()->slSubchannelSize;
    NS_LOG_DEBUG ("Total available symbols for PSSCH = " << availableSymbols);
    //find the minimum available number of contiguous sub-channels in the
    //selected TxOpps
    auto sbChInfo = GetAvailSbChInfo (selectedTxOpps);
    NS_ABORT_MSG_IF (sbChInfo.availSbChIndPerSlot.size () != selectedTxOpps.size (), "subChInfo vector does not have info for all the selected slots");
    do
      {
        assignedSbCh++;
        tbs = GetNrSlAmc ()->CalculateTbSize (dstInfo->GetDstMcs (), sbChSize * assignedSbCh * availableSymbols);
      }
    while (tbs < bufferSize + 5 /*(5 bytes overhead of SCI format 2A)*/ && (sbChInfo.numSubCh - assignedSbCh) > 0);

    //Now, before allocating bytes to LCs we subtract 5 bytes for SCI format 2A
    //since we already took it into account while computing the TB size.
    tbs = tbs - 5 /*(5 bytes overhead of SCI stage 2)*/;

    //NS_LOG_DEBUG ("number of allocated subchannles = " << +assignedSbCh);

    std::vector <uint8_t> startSubChIndexPerSlot = RandSelSbChStart (sbChInfo, assignedSbCh);

    allocated = true;

    auto itsbChIndexPerSlot = startSubChIndexPerSlot.cbegin ();
    auto itTxOpps = selectedTxOpps.cbegin ();

    for (; itTxOpps != selectedTxOpps.cend () && itsbChIndexPerSlot != startSubChIndexPerSlot.cend (); ++itTxOpps, ++itsbChIndexPerSlot)
      {
        NrSlSlotAlloc slotAlloc;
        slotAlloc.sfn = itTxOpps->sfn;
        slotAlloc.dstL2Id = dstInfo->GetDstL2Id ();
        slotAlloc.priority = lcgMap.begin ()->second->GetLcPriority (lcVector.at (0));
        SlRlcPduInfo slRlcPduInfo (lcVector.at (0), tbs);
        slotAlloc.slRlcPduInfo.push_back (slRlcPduInfo);
        slotAlloc.mcs = dstInfo->GetDstMcs ();
        //PSCCH
        slotAlloc.numSlPscchRbs = itTxOpps->numSlPscchRbs;
        slotAlloc.slPscchSymStart = itTxOpps->slPscchSymStart;
        slotAlloc.slPscchSymLength = itTxOpps->slPscchSymLength;
        //PSSCH
        slotAlloc.slPsschSymStart = itTxOpps->slPsschSymStart;
        slotAlloc.slPsschSymLength = availableSymbols;
        slotAlloc.slPsschSubChStart = *itsbChIndexPerSlot;
        slotAlloc.slPsschSubChLength = assignedSbCh;
        slotAlloc.maxNumPerReserve = itTxOpps->slMaxNumPerReserve;
        slotAlloc.ndi = slotAllocList.empty () == true ? 1 : 0;
        // modified
        // if (!slotAlloc.ndi){
        //   break;
        // }
        // end modification
        slotAlloc.rv = GetRv (static_cast<uint8_t>(slotAllocList.size ()));
        if (static_cast<uint16_t>(slotAllocList.size ()) % itTxOpps->slMaxNumPerReserve == 0)
          {
            slotAlloc.txSci1A = true;
            if (slotAllocList.size () + itTxOpps->slMaxNumPerReserve <= selectedTxOpps.size ())
              {
                slotAlloc.slotNumInd = itTxOpps->slMaxNumPerReserve;
              }
            else
              {
                slotAlloc.slotNumInd = selectedTxOpps.size () - slotAllocList.size ();
              }
          }
        else
          {
            slotAlloc.txSci1A = false;
            //Slot, which does not carry SCI 1-A can not indicate future TXs
            slotAlloc.slotNumInd = 0;
          }

        slotAllocList.emplace (slotAlloc);
      }

    lcgMap.begin ()->second->AssignedData (lcVector.at (0), tbs);
    return allocated;
  }
}


std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo>
AiNrSlUeMacSchedulerSimple::RandomlySelectSlots (std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> txOpps)
{
  NS_LOG_FUNCTION (txOpps.size());

  uint8_t totalTx = GetSlMaxTxTransNumPssch ();
  std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> newTxOpps;

  if (txOpps.size () > totalTx)
    {
      while (newTxOpps.size () != totalTx)
        {
          auto txOppsIt = txOpps.begin ();
          // Walk through list until the random element is reached
          std::advance (txOppsIt, m_uniformVariable->GetInteger (0, txOpps.size () - 1));
          //copy the randomly selected slot info into the new list
          newTxOpps.emplace_back (*txOppsIt);
          //erase the selected one from the list
          txOppsIt = txOpps.erase (txOppsIt);
        }
    }
  else
    {
      newTxOpps = txOpps;
    }
  //sort the list by SfnSf before returning
  newTxOpps.sort ();
  NS_ASSERT_MSG (newTxOpps.size () <= totalTx, "Number of randomly selected slots exceeded total number of TX");
  return newTxOpps;
}

std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo>
AiNrSlUeMacSchedulerSimple::RandomlySelectSlots (std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> txOpps, uint8_t totalTx)
{
  NS_LOG_FUNCTION (txOpps.size());

  std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> newTxOpps;

  if (txOpps.size () > totalTx)
    {
      while (newTxOpps.size () != totalTx)
        {
          auto txOppsIt = txOpps.begin ();
          // Walk through list until the random element is reached
          std::advance (txOppsIt, m_uniformVariable->GetInteger (0, txOpps.size () - 1));
          //copy the randomly selected slot info into the new list if it has at least one free subchannel
          if (txOppsIt->occupiedSbCh.size() < GetTotalSubCh() || txOpps.size () <= totalTx)
          {
            newTxOpps.emplace_back (*txOppsIt);
          }
          //erase the selected one from the list, if the itterator is full we dont want to try to use it again
          txOppsIt = txOpps.erase (txOppsIt);
        }
    }
  else
    {
      newTxOpps = txOpps;
    }
  //sort the list by SfnSf before returning
  newTxOpps.sort ();
  NS_ASSERT_MSG (newTxOpps.size () <= totalTx, "Number of randomly selected slots exceeded total number of TX");
  return newTxOpps;
}

AiNrSlUeMacSchedulerSimple::SbChInfo
AiNrSlUeMacSchedulerSimple::GetAvailSbChInfo (std::list <NrSlUeMacSchedSapProvider::NrSlSlotInfo> txOpps)
{
  NS_LOG_FUNCTION (this << txOpps.size ());
  //txOpps are the randomly selected slots for 1st Tx and possible ReTx
  SbChInfo info;
  info.numSubCh = GetTotalSubCh ();
  std::vector <std::vector<uint8_t>> availSbChIndPerSlot;
  for (const auto &it:txOpps)
    {
      std::vector<uint8_t> indexes;
      for (uint8_t i = 0; i < GetTotalSubCh(); i++)
        {
          auto it2 = it.occupiedSbCh.find (i);
          if (it2 == it.occupiedSbCh.end ())
            {
              //available subchannel index(s)
              indexes.push_back (i);
            }
        }
      //it may happen that all sub-channels are occupied
      //remember scheduler can get a slot with all the
      //subchannels occupied because of 3 dB RSRP threshold
      //at UE MAC
      if (indexes.size () == 0)
        {
          for (uint8_t i = 0; i < GetTotalSubCh(); i++)
            {
              indexes.push_back (i);
            }
        }

      NS_ABORT_MSG_IF (indexes.size () == 0, "Available subchannels are zero");

      availSbChIndPerSlot.push_back (indexes);
      uint8_t counter = 0;
      for (uint8_t i = 0; i < indexes.size (); i++)
        {
          uint8_t counter2 = 0;
          uint8_t j = i;
          do
            {
              j++;
              if (j != indexes.size ())
                {
                  counter2++;
                }
              else
                {
                  counter2++;
                  break;
                }
            }
          while (indexes.at (j) == indexes.at (j - 1) + 1);

          counter = std::max (counter, counter2);
        }

      info.numSubCh = std::min (counter, info.numSubCh);
    }

  info.availSbChIndPerSlot = availSbChIndPerSlot;
  for (const auto &it:info.availSbChIndPerSlot)
    {
      NS_ABORT_MSG_IF (it.size () == 0, "Available subchannel size is 0");
    }
  return info;
}

std::vector <uint8_t>
AiNrSlUeMacSchedulerSimple::RandSelSbChStart (SbChInfo sbChInfo, uint8_t assignedSbCh)
{
  NS_LOG_FUNCTION (this << +sbChInfo.numSubCh << sbChInfo.availSbChIndPerSlot.size () << +assignedSbCh);

  std::vector <uint8_t> subChInStartPerSlot;
  uint8_t minContgSbCh = sbChInfo.numSubCh;

  for (const auto &it:sbChInfo.availSbChIndPerSlot)
    {
      if (minContgSbCh == GetTotalSubCh() && assignedSbCh == 1)
        {
          //quick exit
          uint8_t randIndex = static_cast<uint8_t> (m_uniformVariable->GetInteger (0, GetTotalSubCh() - 1));
          subChInStartPerSlot.push_back (randIndex);
        }
      else
        {
          bool foundRandSbChStart = false;
          auto indexes = it;
          do
            {
              NS_ABORT_MSG_IF (indexes.size () == 0, "No subchannels available to choose from");
              uint8_t randIndex = static_cast<uint8_t> (m_uniformVariable->GetInteger (0, indexes.size () - 1));
              NS_LOG_DEBUG ("Randomly drawn index of the subchannel vector is " << +randIndex);
              uint8_t sbChCounter = 0;
              for (uint8_t i = randIndex; i < indexes.size (); i++)
                {
                  sbChCounter++;
                  auto it = std::find (indexes.begin(), indexes.end(), indexes.at (i) + 1);
                  if (sbChCounter == assignedSbCh)
                    {
                      foundRandSbChStart = true;
                      NS_LOG_DEBUG ("Random starting sbch is " << +indexes.at (randIndex));
                      subChInStartPerSlot.push_back (indexes.at (randIndex));
                      break;
                    }
                  if (it == indexes.end())
                    {
                      break;
                    }
                }
            }
          while (foundRandSbChStart == false);
        }
    }

  return subChInStartPerSlot;
}


} //namespace ns3
