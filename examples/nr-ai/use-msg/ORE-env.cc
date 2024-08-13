#include "ORE-env.h"

#include <ns3/ai-module.h>

#include <string>

namespace ns3 {



NS_LOG_COMPONENT_DEFINE("OREENV");

NS_OBJECT_ENSURE_REGISTERED(OREENV);

TypeId
OREENV::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OREENV")
            .SetParent<Object>()
            .AddConstructor<OREENV>()
            // .AddAttribute("DataMode",
            //               "The transmission mode to use for every data packet transmission",
            //               StringValue("OfdmRate6Mbps"),
            //               MakeWifiModeAccessor(&AiConstantRateWifiManager::m_dataMode),
            //               MakeWifiModeChecker())
            // .AddAttribute("ControlMode",
            //               "The transmission mode to use for every RTS packet transmission.",
            //               StringValue("OfdmRate6Mbps"),
            //               MakeWifiModeAccessor(&AiConstantRateWifiManager::m_ctlMode),
            //               MakeWifiModeChecker())
                          ;
    return tid;
}

OREENV::OREENV ()
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  interface->SetIsMemoryCreator(false);
  interface->SetUseVector(false);
  interface->SetHandleFinish(true);
}

OREENV::OREENV (uint16_t id) 
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  interface->SetIsMemoryCreator(false);
  interface->SetUseVector(false);
  interface->SetHandleFinish(true);
}

OREENV::~OREENV()
{
    NS_LOG_FUNCTION(this);
}

void 
OREENV::estimatorUpdate(int imsi, double time, int rsrpThreshold, int occupiedResources, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->imsi = imsi;
  OREInput->time = time;
  OREInput->rsrpThreshold = rsrpThreshold;
  OREInput->occupiedResources = occupiedResources;

  OREInput->estimatorUpdateFlag = estimatorUpdateFlag;
  OREInput->addRecievedPacketFlag = addRecievedPacketFlag;
  OREInput->selectionModeSelectionFlag = selectionModeSelectionFlag;
  OREInput->NSeQueryFlag = NSeQueryFlag;
  OREInput->getNumResourcesSelectedFlag = getNumResourcesSelectedFlag;
  OREInput->getChosenSlotFlag = getChosenSlotFlag;
  OREInput->getChosenSubChannelFlag = getChosenSubChannelFlag;
  msgInterface->CppSendEnd();
  
}

void 
OREENV::addRecievedPacket(int srcRnti,int Rc,int srcSlotNum,int srcScNum, int firstCallFlag, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->srcRnti = srcRnti;
  OREInput->Rc = Rc;
  OREInput->srcSlotNum = srcSlotNum;
  OREInput->srcScNum = srcScNum;
  OREInput->firstCallFlag = firstCallFlag;
  OREInput->estimatorUpdateFlag = estimatorUpdateFlag;
  OREInput->addRecievedPacketFlag = addRecievedPacketFlag;
  OREInput->selectionModeSelectionFlag = selectionModeSelectionFlag;
  OREInput->NSeQueryFlag = NSeQueryFlag;
  OREInput->getNumResourcesSelectedFlag = getNumResourcesSelectedFlag;
  OREInput->getChosenSlotFlag = getChosenSlotFlag;
  OREInput->getChosenSubChannelFlag = getChosenSubChannelFlag;

  msgInterface->CppSendEnd();
}

int 
OREENV::selectionModeSelection(int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->estimatorUpdateFlag = estimatorUpdateFlag;
  OREInput->addRecievedPacketFlag = addRecievedPacketFlag;
  OREInput->selectionModeSelectionFlag = selectionModeSelectionFlag;
  OREInput->NSeQueryFlag = NSeQueryFlag;
  OREInput->getNumResourcesSelectedFlag = getNumResourcesSelectedFlag;
  OREInput->getChosenSlotFlag = getChosenSlotFlag;
  OREInput->getChosenSubChannelFlag = getChosenSubChannelFlag;
  msgInterface->CppSendEnd();

  msgInterface->CppRecvBegin();
  auto OREOutput = msgInterface->GetPy2CppStruct();
  int ret = OREOutput->selectionMode;
  msgInterface->CppRecvEnd();

  return ret;
}

int 
OREENV::NSeQuery(int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->estimatorUpdateFlag = estimatorUpdateFlag;
  OREInput->addRecievedPacketFlag = addRecievedPacketFlag;
  OREInput->selectionModeSelectionFlag = selectionModeSelectionFlag;
  OREInput->NSeQueryFlag = NSeQueryFlag;
  OREInput->getNumResourcesSelectedFlag = getNumResourcesSelectedFlag;
  OREInput->getChosenSlotFlag = getChosenSlotFlag;
  OREInput->getChosenSubChannelFlag = getChosenSubChannelFlag;
  msgInterface->CppSendEnd();

  msgInterface->CppRecvBegin();
  auto OREOutput = msgInterface->GetPy2CppStruct();
  int ret = OREOutput->NSe;
  msgInterface->CppRecvEnd();

  return ret;
}

int 
OREENV::getNumResourcesSelected(int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->estimatorUpdateFlag = estimatorUpdateFlag;
  OREInput->addRecievedPacketFlag = addRecievedPacketFlag;
  OREInput->selectionModeSelectionFlag = selectionModeSelectionFlag;
  OREInput->NSeQueryFlag = NSeQueryFlag;
  OREInput->getNumResourcesSelectedFlag = getNumResourcesSelectedFlag;
  OREInput->getChosenSlotFlag = getChosenSlotFlag;
  OREInput->getChosenSubChannelFlag = getChosenSubChannelFlag;
  msgInterface->CppSendEnd();

  msgInterface->CppRecvBegin();
  auto OREOutput = msgInterface->GetPy2CppStruct();
  int ret = OREOutput->numResourcesSelected;
  msgInterface->CppRecvEnd();

  return ret;
}

int 
OREENV::getChosenSlot(int index, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->index = index;
  OREInput->estimatorUpdateFlag = estimatorUpdateFlag;
  OREInput->addRecievedPacketFlag = addRecievedPacketFlag;
  OREInput->selectionModeSelectionFlag = selectionModeSelectionFlag;
  OREInput->NSeQueryFlag = NSeQueryFlag;
  OREInput->getNumResourcesSelectedFlag = getNumResourcesSelectedFlag;
  OREInput->getChosenSlotFlag = getChosenSlotFlag;
  OREInput->getChosenSubChannelFlag = getChosenSubChannelFlag;
  msgInterface->CppSendEnd();

  msgInterface->CppRecvBegin();
  auto OREOutput = msgInterface->GetPy2CppStruct();
  int ret = OREOutput->chosenSlot;
  msgInterface->CppRecvEnd();

  return ret;
}

int 
OREENV::getChosenSubChannel(int index, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->index = index;
  OREInput->estimatorUpdateFlag = estimatorUpdateFlag;
  OREInput->addRecievedPacketFlag = addRecievedPacketFlag;
  OREInput->selectionModeSelectionFlag = selectionModeSelectionFlag;
  OREInput->NSeQueryFlag = NSeQueryFlag;
  OREInput->getNumResourcesSelectedFlag = getNumResourcesSelectedFlag;
  OREInput->getChosenSlotFlag = getChosenSlotFlag;
  OREInput->getChosenSubChannelFlag = getChosenSubChannelFlag;
  msgInterface->CppSendEnd();

  msgInterface->CppRecvBegin();
  auto OREOutput = msgInterface->GetPy2CppStruct();
  int ret = OREOutput->chosenSubChannel;
  msgInterface->CppRecvEnd();

  return ret;
}

void
OREENV::passSensingData(int imsi, double time, int rsrpThreshold, int occupiedResources, double encodedSrcRnti, double encodedRc, double encodedSrcSlot, double encodedSrcSc, int sensingDataFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->imsi = imsi;
  OREInput->time = time;
  OREInput->rsrpThreshold = rsrpThreshold;
  OREInput->occupiedResources = occupiedResources;
  OREInput->encodedSrcRnti = encodedSrcRnti;
  OREInput->encodedRc = encodedRc;
  OREInput->encodedSrcSlot = encodedSrcSlot;
  OREInput->encodedSrcSc = encodedSrcSc;
  OREInput->sensingDataFlag = sensingDataFlag;

  msgInterface->CppSendEnd();
  
  
}

double
OREENV::getResourceSelections(int sensingDataFlag)
{
  NS_LOG_FUNCTION(this);
  auto interface = Ns3AiMsgInterface::Get();
  Ns3AiMsgInterfaceImpl<OREEnvStruct, OREActStruct>* msgInterface =
        interface->GetInterface<OREEnvStruct, OREActStruct>();

  msgInterface->CppSendBegin();
  auto OREInput = msgInterface->GetCpp2PyStruct();
  OREInput->sensingDataFlag = sensingDataFlag;
  msgInterface->CppSendEnd();

  msgInterface->CppRecvBegin();
  auto OREOutput = msgInterface->GetPy2CppStruct();
  int ret = OREOutput->encodedSelectionInstructions;
  msgInterface->CppRecvEnd();

  return ret;
}





}// namespace ns3



















