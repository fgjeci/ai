// #pragma once
// #include "ns3/ns3-ai-rl.h"
// #include <ns3/ai-module.h>

#ifndef ORE_ENV_H
#define ORE_ENV_H

#include "ns3/object.h"


namespace ns3 {

struct OREEnvStruct
{
	int imsi;
	double time;
	int occupiedResources;
	int rsrpThreshold;
	int srcRnti;
	int Rc;
	int srcSlotNum;
	int srcScNum;
	int index;
	int estimatorUpdateFlag;
	int addRecievedPacketFlag;
	int selectionModeSelectionFlag;
	int getNumResourcesSelectedFlag;
	int getChosenSlotFlag;
	int getChosenSubChannelFlag;
	int NSeQueryFlag;
	int firstCallFlag;
	double encodedSrcRnti;
	double encodedRc;
	double encodedSrcSlot;
	double encodedSrcSc;
	int sensingDataFlag;
};

struct OREActStruct
{
	int selectionMode;
	int NSe;
	int numResourcesSelected;
	int chosenSlot;
	int chosenSubChannel;
	double encodedSelectionInstructions;
};

class OREENV: public Object
{
	public:
		
		
		OREENV();
		OREENV (uint16_t id, std::string plmnId, std::string reType);
		virtual ~OREENV();
		/**
		 * \brief Get the type ID.
		 * \return the object TypeId
		 */
		static TypeId GetTypeId();
		void estimatorUpdate(int imsi, double time, int rsrpThreshold, int occupiedResources, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag);//called during sensing to update \hat{\rho}_{UE} and d_{edge}
		void addRecievedPacket(int srcRnti,int Rc,int srcSlotNum,int srcScNum, int firstCallFlag, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag);
		int selectionModeSelection(int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag);
		int NSeQuery(int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag);
		int getNumResourcesSelected(int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag);
		int getChosenSlot(int index, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag);
		int getChosenSubChannel(int index, int estimatorUpdateFlag, int addRecievedPacketFlag, int selectionModeSelectionFlag, int NSeQueryFlag, int getNumResourcesSelectedFlag, int getChosenSlotFlag, int getChosenSubChannelFlag);
		void passSensingData(int imsi, double time, int rsrpThreshold, int occupiedResources, double encodedSrcRnti, double encodedRc, double encodedSrcSlot, double encodedSrcSc, int sensingDataFlag);
		double getResourceSelections(int sensningDataFlag);
	private:
		// std::string m_ltePlmnId;
		// std::string m_reType;

};
}// namespace ns3

#endif /* NR_UE_MAC_H */