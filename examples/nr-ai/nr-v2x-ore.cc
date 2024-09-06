/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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

/**
 * \ingroup examples
 * \file cttc-nr-v2x-demo-simple.cc
 * \brief A cozy, simple, NR V2X demo (in a tutorial style)
 *
 * This example describes how to setup an NR sidelink out-of-coverage simulation
 * using the 3GPP channel model from TR 37.885. This example simulates a
 * simple topology consist of 2 UEs, where UE-1 transmits, and UE-2 receives.
 *
 * Have a look at the possible parameters to know what you can configure
 * through the command line.
 *
 * With the default configuration, the example will create a flow that will
 * go through a subband or bandwidth part. For that,
 * specifically, one band with a single CC, and one bandwidth part is used.
 *
 * The example will print on-screen the average Packet Inter-Reception (PIR)
 * type 2 computed as defined in 37.885. Moreover, it saves MAC and PHY layer
 * traces in a sqlite3 database using ns-3 stats module. Moreover, since there
 * is only one transmitter in the scenario, sensing is by default not enabled.
 *
 * \code{.unparsed}
$ ./ns3 run "cttc-nr-v2x-demo-simple --help"
    \endcode
 *
 */

/*
 * Include part. Often, you will have to include the headers for an entire module;
 * do that by including the name of the module you need with the suffix "-module.h".
 */

#include "ns3/core-module.h"
#include "ns3/config-store.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/nr-module.h"
#include "ns3/lte-module.h"
#include "ns3/stats-module.h"
#include "ns3/config-store-module.h"
#include "ns3/log.h"
#include "ns3/ue-mac-pscch-tx-output-stats.h"
#include "ns3/ue-mac-pssch-tx-output-stats.h"
#include "ns3/ue-phy-pscch-rx-output-stats.h"
#include "ns3/ue-phy-pssch-rx-output-stats.h"
#include "ns3/ue-to-ue-pkt-txrx-output-stats.h"
#include "ns3/ue-v2x-scheduling-stats.h"
#include "ns3/antenna-module.h"
#include "ns3/nr-sl-comm-resource-pool.h"
#include <iomanip>
#include <ns3/random-variable-stream.h>
#include <ns3/node-list.h>
#include <ns3/nr-ue-net-device.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/buildings-module.h>
#include "ns3/parameters-config.h"

// inclue old reports
#include "ns3/sl-stats-helper.h"
#include "ns3/sl-packet-trace-stats.h"
#include "ns3/sl-power-output-stats.h"
#include "ns3/sl-slot-stats.h"
#include "ns3/sl-slot-output-stats.h"
#include "ns3/sl-slot-ctrl-output-stats.h"
#include "ns3/sl-rb-output-stats.h"
#include "ns3/sl-sinr-output-stats.h"
#include "ns3/sl-flow-monitor-output-stats.h"
#include "ns3/sl-downlink-tb-size-stats.h"
#include "ns3/sl-uplink-tb-size-stats.h"
#include "ns3/sl-ctrl-ul-dci-stats.h"
#include "ns3/sl-mac-sr-stats.h"
#include "ns3/sl-mac-rlc-buffer-status.h"
#include "ns3/sl-ue-bsr-stats.h"
#include "ns3/sl-ue-pss-received-stats.h"
#include "ns3/nr-dl-scheduling-stats.h"
#include "ns3/sl-ctrl-msgs-stats.h"
#include "ns3/sl-mobility-stats.h"
#include "ns3/sl-scheduling-stats.h"

#include "ns3/ns2-mobility-helper.h"
#include "ns3/sl-stats-helper.h"
#include "ns3/sl-mobility-stats.h"

#include "ns3/sl-sfnsf.pb.h"

#include "use-msg/ai-nr-sl-ue-mac-scheduler-simple.h"
#include "use-msg/ai-nr-ue-mac.h"

#include "ns3/sl-spectrum-stats.h"

/*
 * Use, always, the namespace ns3. All the NR classes are inside such namespace.
 */
using namespace ns3;

/*
 * With this line, we will be able to see the logs of the file by enabling the
 * component "CttcNrV2xDemoSimple", in this way:
 *
 * $ export NS_LOG="CttcNrV2xDemoSimple=level_info|prefix_func|prefix_time"
 */
NS_LOG_COMPONENT_DEFINE ("NrV2xORE");


std::ofstream m_ueRrcStateFile;
std::ofstream m_gnbRrcStateFile;

/// Map each of UE Manager states to its string representation.
static const std::string g_ueManagerStateName[UeManager::NUM_STATES] =
{
  "INITIAL_RANDOM_ACCESS",
  "CONNECTION_SETUP",
  "CONNECTION_REJECTED",
  "ATTACH_REQUEST",
  "CONNECTED_NORMALLY",
  "CONNECTION_RECONFIGURATION",
  "CONNECTION_REESTABLISHMENT",
  "HANDOVER_PREPARATION",
  "HANDOVER_JOINING",
  "HANDOVER_PATH_SWITCH",
  "HANDOVER_LEAVING",
};

/**
 * \param s The UE manager state.
 * \return The string representation of the given state.
 */
static const std::string & gnbToStringState (UeManager::State s)
{
  return g_ueManagerStateName[s];
}

/// Map each of UE RRC states to its string representation.
static const std::string g_ueRrcStateName[LteUeRrc::NUM_STATES] =
{
  "IDLE_START",
  "IDLE_CELL_SEARCH",
  "IDLE_WAIT_MIB_SIB1",
  "IDLE_WAIT_MIB",
  "IDLE_WAIT_SIB1",
  "IDLE_CAMPED_NORMALLY",
  "IDLE_WAIT_SIB2",
  "IDLE_RANDOM_ACCESS",
  "IDLE_CONNECTING",
  "CONNECTED_NORMALLY",
  "CONNECTED_HANDOVER",
  "CONNECTED_PHY_PROBLEM",
  "CONNECTED_REESTABLISHING"
};

/**
 * \param s The UE RRC state.
 * \return The string representation of the given state.
 */
static const std::string & ueToStringState (LteUeRrc::State s)
{
  return g_ueRrcStateName[s];
}

void SwitchStateUe(std::string path, uint64_t imsi, uint16_t cellId, uint16_t rnti, ns3::LteUeRrc::State oldState, ns3::LteUeRrc::State newState){
	if (!m_ueRrcStateFile.is_open ())
      {
        std::string m_ueRrcStateFileName = path + "UeRrcStateTrace.txt";
        m_ueRrcStateFile.open (m_ueRrcStateFileName.c_str ());
        m_ueRrcStateFile << "Time" << "\t" << "Imsi" << "\t"<< "CellId"  << "\t" << "Rnti" <<
                                "\t" << "OldState" << "\t" << "NewState" << std::endl;

        if (!m_ueRrcStateFile.is_open ())
          {
            NS_FATAL_ERROR ("Could not open tracefile");
          }
      }

  m_ueRrcStateFile << Simulator::Now ().GetNanoSeconds () << // (double) 1e9
                          "\t" << imsi << "\t" << (uint32_t) cellId <<
                          "\t" << (uint32_t) rnti <<
                          "\t" << ueToStringState(oldState) <<
                          "\t" << ueToStringState(newState) << std::endl;
}

void SwitchStateGnb(std::string path, uint64_t imsi, uint16_t cellId, uint16_t rnti, ns3::UeManager::State oldState, ns3::UeManager::State newState){
	// std::cout << "Adding state to switch state gnb" << std::endl;
	if (!m_gnbRrcStateFile.is_open ())
      {
        std::string m_gnbRrcStateFileName = path + "GnbRrcStateTrace.txt";
        m_gnbRrcStateFile.open (m_gnbRrcStateFileName.c_str ());
        m_gnbRrcStateFile << "Time" << "\t" << "Imsi" << "\t"<< "CellId"  << "\t" << "Rnti" <<
                                "\t" << "OldState" << "\t" << "NewState" << std::endl;

        if (!m_gnbRrcStateFile.is_open ())
          {
            NS_FATAL_ERROR ("Could not open tracefile");
          }
      }

  	m_gnbRrcStateFile << Simulator::Now ().GetNanoSeconds () << // (double) 1e9 
                          "\t" << imsi << "\t" << (uint32_t) cellId <<
                          "\t" << (uint32_t) rnti <<
                          "\t" << gnbToStringState(oldState) <<
                          "\t" << gnbToStringState(newState) << std::endl;
}

void NewUeContext(Ptr<LteEnbRrc> gnbRrc, std::string path, uint16_t cellId, uint16_t rnti){
	// having gnb rrc and new added rnti we can activate the ue manager trace
	// std::cout << "New context created with cellid " <<cellId << " and rnti " <<rnti << std::endl;
	gnbRrc->GetUeManager(rnti)->TraceConnectWithoutContext("StateTransition", MakeBoundCallback(&SwitchStateGnb, path));
}

/*
 * Global variables to count TX/RX packets and bytes.
 */

uint32_t rxByteCounter = 0; //!< Global variable to count RX bytes
uint32_t txByteCounter = 0; //!< Global variable to count TX bytes
uint32_t rxPktCounter = 0; //!< Global variable to count RX packets
uint32_t txPktCounter = 0; //!< Global variable to count TX packets

/**
 * \brief Method to listen the packet sink application trace Rx.
 * \param packet The packet
 */
void ReceivePacket (Ptr<const Packet> packet, const Address &)
{
  rxByteCounter += packet->GetSize();
  rxPktCounter++;
}

/**
 * \brief Method to listen the transmitting application trace Tx.
 * \param packet The packet
 */
void TransmitPacket (Ptr<const Packet> packet)
{
  txByteCounter += packet->GetSize();
  txPktCounter++;
}

/*
 * Global variable used to compute PIR
 */
uint64_t pirCounter = 0; //!< counter to count how many time we computed the PIR. It is used to compute average PIR
Time lastPktRxTime; //!< Global variable to store the RX time of a packet
Time pir; //!< Global varible to store PIR value

/**
 * \brief This method listens to the packet sink application trace Rx.
 * \param packet The packet
 * \param from The address of the transmitter
 */
void ComputePir (Ptr<const Packet> packet, const Address &)
{
  if (pirCounter == 0 && lastPktRxTime.GetSeconds () == 0.0)
    {
      //this the first packet, just store the time and get out
      lastPktRxTime = Simulator::Now ();
      return;
    }
  pir = pir + (Simulator::Now () - lastPktRxTime);
  lastPktRxTime = Simulator::Now ();
  pirCounter++;
}

int 
main (int argc, char *argv[])
{
  /*
   * Variables that represent the parameters we will accept as input by the
   * command line. Each of them is initialized with a default value.
   */
  // Scenario parameters (that we will use inside this script):
  uint16_t interUeDistance = 2; //meters
  bool logging = true;
  bool OReExp = false;//the object used to perform both O-Re and D-Re experiments is the same, the difference is in the run.py file, not the c++ script



  // Traffic parameters (that we will use inside this script:)
  bool useIPv6 = false; // default IPV4
  uint32_t udpPacketSizeBe = 200;
  double dataRateBe = 16; //16 kilobits per second

  // Simulation parameters.
  double simTimeNumber = 0.3;
  //Sidelink bearers activation time
  Time slBearersActivationTime = Seconds (0.1);


  // NR parameters. We will take the input from the command line, and then we
  // will pass them inside the NR module. 
  uint16_t numerologyBwpSl = 2;
  double centralFrequencyBandSl = 5.89e9; // band n47  TDD //Here band is analogous to channel
  uint16_t bandwidthBandSl = 400; //Multiple of 100 KHz; 400 = 40 MHz
  double txPower = 23; //dBm

  // Where we will store the output files.
  // std::string simTag = "default";
  // std::string outputDir = "./";

  // ConfigStore config;
  // config.ConfigureDefaults ();

  // modified
  std::string ltePlmnId = "111";
	std::string ricE2TermIpAddress = "10.0.2.10";
	uint16_t ricE2TermPortNumber = 36422;
	uint16_t e2startingPort = 38470;
  std::string simTag = "urban";
	std::string outputDir = "/storage/franci/simulations/ns3-v2x/nr-v2x-mode1/";
  bool e2cuCp = false;
  bool e2du = false;
  bool isXappEnabled = false;
  uint8_t schedulingType = 1;// default is Scheduled (GNB) = 1, ue Selected (2) and ORAN SELECTED (3)
  double ueDiscCenterXPosition = 0;
  double ueDiscCenterYPosition = 0;
  uint16_t ueNum = 2; // ue number 

  std::string mobilityTraceFile = outputDir + simTag +  "/new_mobility.tcl";
  std::string buildingsFile = outputDir + simTag + "/buildings_pos.csv";
  bool isUrbanScenario = true;

  uint32_t interMessageSleepMs = 1000;

  std::string nrUeMacTypeName = "NrUeMac";

  std::string reType = "DRE";

  uint16_t reservationPeriod = 100; // in ms
  bool enableSensing = true;
  uint16_t t1 = 2;
  uint16_t t2 = 33;
  int slThresPsschRsrp = -110;//-140.4575749056067;//-91.70696227169;//-128;

  // end modification

  /*
   * From here, we instruct the ns3::CommandLine class of all the input parameters
   * that we may accept as input, as well as their description, and the storage
   * variable.
   */
  CommandLine cmd (__FILE__);

  cmd.AddValue ("interUeDistance",
                "The distance among the UEs in the topology",
                interUeDistance);
  cmd.AddValue ("logging",
                "Enable logging",
                logging);
  cmd.AddValue ("useIPv6",
                "Use IPv6 instead of IPv4",
                useIPv6);
  cmd.AddValue ("packetSizeBe",
                "packet size in bytes to be used by best effort traffic",
                udpPacketSizeBe);
  cmd.AddValue ("dataRateBe",
                "The data rate in kilobits per second for best effort traffic",
                dataRateBe);
  cmd.AddValue ("simTime",
                "Simulation time in seconds",
                simTimeNumber);
  cmd.AddValue ("slBearerActivationTime",
                "Sidelik bearer activation time in seconds",
                slBearersActivationTime);
  cmd.AddValue ("numerologyBwpSl",
                "The numerology to be used in sidelink bandwidth part",
                numerologyBwpSl);
  cmd.AddValue ("centralFrequencyBandSl",
                "The central frequency to be used for sidelink band/channel",
                centralFrequencyBandSl);
  cmd.AddValue ("bandwidthBandSl",
                "The system bandwidth to be used for sidelink",
                bandwidthBandSl);
  cmd.AddValue ("txPower",
                "total tx power in dBm",
                txPower);
  cmd.AddValue ("simTag",
                "tag to be appended to output filenames to distinguish simulation campaigns",
                simTag);
  cmd.AddValue ("outputDir",
                "directory where to store simulation results",
                outputDir);
  cmd.AddValue ("ueNumber",
                "Number of ues in the simulation. Default is 50",
                ueNum);
  
  // modified

  cmd.AddValue("ricE2TermIpAddress",
				 "The ip address to reach the termination point of ric",
				 ricE2TermIpAddress);
  cmd.AddValue("ricE2TermPortNumber",
				 "The port number of the E2 termination endpoint",
				 ricE2TermPortNumber);
  cmd.AddValue("ltePlmnId",
				 "Plmn id of the lte coordinator, to distinguish different simulations campaigns",
				 ltePlmnId);
  cmd.AddValue("e2StartingPort",
				 "starting port number for the gnb e2 termination; destination is same",
				 e2startingPort);
  cmd.AddValue("isXappEnabled",
				 "Define if the simulation has the support of Xapp",
				 isXappEnabled);
  cmd.AddValue("ueSchedulingType",
				 "Scheduling type: UE or ORAN",
				 schedulingType);
  cmd.AddValue("ueDiscCenterXPosition",
				 "The x coordinate of the disc center of the ue position",
				 ueDiscCenterXPosition);
  cmd.AddValue("ueDiscCenterYPosition",
				 "The y coordinate of the disc center of the ue position",
				 ueDiscCenterYPosition);
  cmd.AddValue("mobilityTraceFile",
          "Ns2 movement trace file",
          mobilityTraceFile);
  cmd.AddValue("buildingsFile",
          "Buildings file",
          buildingsFile);
  cmd.AddValue("isUrbanScenario",
              "Is urban scenario",
              isUrbanScenario);
  cmd.AddValue("interMessageSleepMs",
              "intermessage sleep in ms",
              interMessageSleepMs);
  cmd.AddValue ("OReExp","flag for ORe experiments, false turns them off", OReExp);
              
  cmd.AddValue ("nrUeMacType", "NrUeMac or AiNrUeMac type", nrUeMacTypeName);
  
  cmd.AddValue ("reType", "DRE or ORE simulation type", reType);

  cmd.AddValue ("ReservationPeriod",
                "The resource reservation period in ms",
                reservationPeriod);
  cmd.AddValue ("enableSensing",
                "If true, it enables the sensing based resource selection for "
                "SL, otherwise, no sensing is applied",
                enableSensing);
  cmd.AddValue ("t1",
                "The start of the selection window in physical slots, "
                "accounting for physical layer processing delay",
                t1);
  cmd.AddValue ("t2",
                "The end of the selection window in physical slots",
                t2);
  cmd.AddValue ("slThresPsschRsrp",
                "A threshold in dBm used for sensing based UE autonomous resource selection",
                slThresPsschRsrp);

  std::string nrUeMacTypeNameComplete = "ns3::" +nrUeMacTypeName;

  std::cout << "ue mac type " << nrUeMacTypeNameComplete << std::endl;

  // exit(1);

  // end modification
  
  // Parse the command line
  cmd.Parse (argc, argv);

  // ParametersConfig::EnableTraces();


  Time simTime = Seconds (simTimeNumber);

  double indicationPeriodicity = 0.01; // seconds


  NrSlCommResourcePool::SchedulingType schedulingTypeValue = static_cast<NrSlCommResourcePool::SchedulingType>(schedulingType);
  // std::cout << "Scheduling type " << schedulingTypeValue << std::endl;
  Config::SetDefault ("ns3::LteUeRrc::V2XSchedulingType", EnumValue (schedulingTypeValue));
  Config::SetDefault ("ns3::LteEnbRrc::V2XSchedulingType", EnumValue (schedulingTypeValue));

  if (schedulingTypeValue ==NrSlCommResourcePool::ORAN_SELECTED){
    e2cuCp = true;
  }


  // Config::SetDefault ("ns3::LteEnbNetDevice::ControlFileName", StringValue (controlFilename));
  Config::SetDefault ("ns3::LteEnbNetDevice::E2Periodicity", DoubleValue (indicationPeriodicity));
  Config::SetDefault ("ns3::NrGnbNetDevice::E2Periodicity", DoubleValue (indicationPeriodicity));

  Config::SetDefault ("ns3::NrGnbNetDevice::EnableCuCpReport", BooleanValue (e2cuCp));
  Config::SetDefault ("ns3::NrGnbNetDevice::EnableDuReport", BooleanValue (e2du));

  Config::SetDefault ("ns3::NrGnbNetDevice::InterMessageSleepMs", UintegerValue (interMessageSleepMs));

  Config::SetDefault ("ns3::NrHelper::E2TermIp", StringValue (ricE2TermIpAddress));
  Config::SetDefault ("ns3::NrHelper::E2Port", UintegerValue (ricE2TermPortNumber));

  Config::SetDefault ("ns3::NrHelper::E2LocalPort", UintegerValue (e2startingPort));

  Config::SetDefault ("ns3::NrHelper::PlmnId", StringValue (ltePlmnId));
  Config::SetDefault ("ns3::NrGnbNetDevice::PlmnId", StringValue (ltePlmnId));

  Config::SetDefault ("ns3::NrHelper::E2ModeNr", BooleanValue (isXappEnabled));

  Config::SetDefault ("ns3::NrGnbNetDevice::TracesPath",
                      StringValue(outputDir + simTag + "/"));
  Config::SetDefault ("ns3::LteEnbNetDevice::TracesPath",
                      StringValue(outputDir + simTag + "/"));
  Config::SetDefault ("ns3::NrUeMac::TracesPath",
                      StringValue(outputDir + simTag + "/"));
  Config::SetDefault ("ns3::AiNrSlUeMacSchedulerSimple::TracesPath",
                      StringValue(outputDir + simTag + "/"));

  Config::SetDefault ("ns3::AiNrUeMac::OReExp", BooleanValue(OReExp));
  Config::SetDefault ("ns3::AiNrSlUeMacSchedulerSimple::OReExp", BooleanValue(OReExp));

  std::cout << "plmn id " << ltePlmnId << " " << reType <<std::endl;

  Config::SetDefault ("ns3::AiNrSlUeMacSchedulerSimple::PlmnId", StringValue(ltePlmnId));
  Config::SetDefault ("ns3::AiNrSlUeMacSchedulerSimple::REType", StringValue(reType));
  Config::SetDefault ("ns3::AiNrUeMac::PlmnId", StringValue(ltePlmnId));
  Config::SetDefault ("ns3::AiNrUeMac::REType", StringValue(reType));


  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds(100)));

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));

  if (schedulingTypeValue ==NrSlCommResourcePool::ORAN_SELECTED){
    Config::SetDefault ("ns3::LteRlcUm::ReorderingTimer", TimeValue(MilliSeconds(1)));
  }

  // ParametersConfig::CreateTracesDir(params);

  // Final simulation time is the addition of:
  //simTime + slBearersActivationTime + 10 ms additional delay for UEs to activate the bearers
  Time finalSlBearersActivationTime = slBearersActivationTime + Seconds (0.01);
  Time finalSimTime = simTime + finalSlBearersActivationTime;
  std::cout << "Final Simulation duration " << finalSimTime.GetSeconds () << std::endl;

  NS_ABORT_IF (centralFrequencyBandSl > 6e9);

  if (logging)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL | LOG_DEBUG | LOG_LEVEL_FUNCTION);
      // LogComponentEnable ("UdpClient", logLevel);
      LogComponentEnable ("NrV2xORE", logLevel);
      LogComponentEnable ("NrHelper", logLevel);
      LogComponentEnable ("NrSlHelper", logLevel);
      // LogComponentEnable ("OREENV", logLevel);

      // LogComponentEnable ("AiNrSlUeMacSchedulerSimple", logLevel);
      // LogComponentEnable ("NrSlUeMacSchedulerSimple", logLevel);
      
      // LogComponentEnable ("NrGnbMac", logLevel);
      // LogComponentEnable ("NrGnbPhy", logLevel);
      // LogComponentEnable ("BandwidthPartGnb", logLevel);
      
      // LogComponentEnable ("AiNrUeMac", logLevel);
      // LogComponentEnable ("NrUeMac", logLevel);
      // LogComponentEnable ("NrUePhy", logLevel);
      // LogComponentEnable ("NrPhy", logLevel);
      // LogComponentEnable ("NrSpectrumPhy", logLevel);
      // LogComponentEnable ("NrSlUeMacHarq", logLevel);
      
      // LogComponentEnable ("E2Termination", logLevel);
      // LogComponentEnable ("NrGnbNetDevice", logLevel);
      // LogComponentEnable("LteRlcUm", logLevel);
      // LogComponentEnable("LtePdcp", logLevel);
      // LogComponentEnable("LteUeRrc", logLevel);
      // LogComponentEnable ("NrSlEnbRrc", logLevel);
      // LogComponentEnable ("NrSlUeRrc", logLevel);
      // LogComponentEnable ("NrBearerStatsSimple", logLevel);
      // LogComponentEnable ("NrBearerStatsConnector", logLevel);

    
      // LogComponentEnable ("NrSlEnbMacScheduler", logLevel);
      // LogComponentEnable ("NrSlEnbMacSchedulerLCG", logLevel);
      // LogComponentEnable ("NrSlEnbMacSchedulerNs3", logLevel);
      // LogComponentEnable ("NrSlEnbMacSchedulerTdma", logLevel);
      // LogComponentEnable ("NrSlEnbMacSchedulerTdmaRR", logLevel);
      // LogComponentEnable ("NrSlEnbMacSchedulerDstInfo", logLevel);

    }

  /*
   * Default values for the simulation. We are progressively removing all
   * the instances of SetDefault, but we need it for legacy code (LTE)
   */
  
  Ns2MobilityHelper ns2 = Ns2MobilityHelper(mobilityTraceFile);

  NodeContainer enbNodes;
  enbNodes.Create (7);

  NodeContainer ueVoiceContainer;
  ueVoiceContainer.Create (ueNum);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < ueNum; i++)
    {
      positionAllocUe->Add (Vector (interUeDistance * i, 0.0, 1.5));
    }
  mobility.SetPositionAllocator (positionAllocUe);
  mobility.Install (ueVoiceContainer);

  double rho = 500;
  // Ptr<UniformDiscPositionAllocator> uePositionAlloc = CreateObject<UniformDiscPositionAllocator> ();
  // uePositionAlloc->SetX(ueDiscCenterXPosition);
  // uePositionAlloc->SetY(ueDiscCenterYPosition);
  // uePositionAlloc->SetZ(1.5);
  // uePositionAlloc->SetRho (rho);
  // Ptr<UniformRandomVariable> speed = CreateObject<UniformRandomVariable> ();
  // speed->SetAttribute ("Min", DoubleValue (0.5));
  // speed->SetAttribute ("Max", DoubleValue (1.8));
  // mobility.SetMobilityModel ("ns3::RandomWalk2dOutdoorMobilityModel", "Speed",
  //                               PointerValue (speed), "Bounds",
  //                               RectangleValue (Rectangle (ueDiscCenterXPosition-rho, ueDiscCenterXPosition+rho, ueDiscCenterYPosition-rho, ueDiscCenterYPosition+rho)));
  // mobility.SetPositionAllocator (uePositionAlloc);
  
  // mobility.Install (ueVoiceContainer);
  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();
  nrHelper->SetAttribute("TracesPath", StringValue(outputDir + simTag + "/"));

  // Put the pointers inside nrHelper
  NS_LOG_DEBUG("ue mac type complete " << nrUeMacTypeNameComplete
              << " nr ue mac " << NrUeMac::GetTypeId().GetName()
              << nrUeMacTypeNameComplete.compare(NrUeMac::GetTypeId().GetName())); 
  nrHelper->SetEpcHelper (epcHelper);
  if(nrUeMacTypeNameComplete.compare(NrUeMac::GetTypeId().GetName())){
    nrHelper->SetUeMacTypeId (NrUeMac::GetTypeId());
  }else{
    nrHelper->SetUeMacTypeId(AiNrUeMac::GetTypeId());
  }

  BandwidthPartInfoPtrVector allBwps;
  CcBwpCreator ccBwpCreator;
  const uint8_t numCcPerBand = 1;

  BandwidthPartInfo::Scenario scenario {BandwidthPartInfo::V2V_Highway};
  if (isUrbanScenario){
    scenario = BandwidthPartInfo::V2V_Urban;  
  }

  CcBwpCreator::SimpleOperationBandConf bandConfSl (centralFrequencyBandSl, bandwidthBandSl, numCcPerBand, scenario);

  // By using the configuration created, it is time to make the operation bands
  OperationBandInfo bandSl = ccBwpCreator.CreateOperationBandContiguousCc (bandConfSl);

  
  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (100)));
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

  nrHelper->InitializeOperationBand (&bandSl);
  allBwps = CcBwpCreator::GetAllBwps ({bandSl});

  Packet::EnableChecking ();
  Packet::EnablePrinting ();

  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));

  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (1));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  nrHelper->SetUePhyAttribute ("TxPower", DoubleValue (txPower));

  //NR Sidelink attribute of UE MAC, which are would be common for all the UEs
  nrHelper->SetUeMacAttribute ("EnableSensing", BooleanValue (enableSensing));
  nrHelper->SetUeMacAttribute ("T1", UintegerValue (t1));
  nrHelper->SetUeMacAttribute ("T2", UintegerValue (t2));
  nrHelper->SetUeMacAttribute ("ActivePoolId", UintegerValue (0));
  nrHelper->SetUeMacAttribute ("ReservationPeriod", TimeValue (MilliSeconds(reservationPeriod)));
  nrHelper->SetUeMacAttribute ("NumSidelinkProcess", UintegerValue (4));
  nrHelper->SetUeMacAttribute ("EnableBlindReTx", BooleanValue (true));
  nrHelper->SetUeMacAttribute ("SlThresPsschRsrp", IntegerValue (slThresPsschRsrp));


  // nrHelper->SetUeMacAttribute ("PlmnId", StringValue(ltePlmnId));
  // nrHelper->SetUeMacAttribute ("REType", StringValue(reType));


  uint8_t bwpIdForGbrMcptt = 0;

  nrHelper->SetBwpManagerTypeId (TypeId::LookupByName ("ns3::NrSlBwpManagerUe"));
  nrHelper->SetUeBwpManagerAlgorithmAttribute ("GBR_MC_PUSH_TO_TALK", UintegerValue (bwpIdForGbrMcptt));

  std::set<uint8_t> bwpIdContainer;
  bwpIdContainer.insert (bwpIdForGbrMcptt);

  // create single gnb device to get the messages
  // same carrier as sl to enable msg exchange
  // double frequencyGnb = 5.89e9; // 28e9; // central frequency
  // double bandwidthGnb = 400 * 1e5; // 100e6; //bandwidth
  // const uint8_t numCcPerBandGnb = 1;
  // enum BandwidthPartInfo::Scenario scenarioEnumGnb = BandwidthPartInfo::UMa;
  // CcBwpCreator::SimpleOperationBandConf bandConfGnb (frequencyGnb, bandwidthGnb, numCcPerBandGnb, scenarioEnumGnb);
  // OperationBandInfo bandGnb = ccBwpCreator.CreateOperationBandContiguousCc (bandConfGnb);
  //Initialize channel and pathloss, plus other things inside band.
  // nrHelper->InitializeOperationBand (&bandGnb);

  // BandwidthPartInfoPtrVector allBwpsGnb = CcBwpCreator::GetAllBwps ({bandGnb});

  
  // position the base stations
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  if (isUrbanScenario){
    enbPositionAlloc->Add (Vector (302.97, 302.10, 35));
    enbPositionAlloc->Add (Vector (297.62, 533.08, 35));
    enbPositionAlloc->Add (Vector (30.28, 415.45, 35));
    enbPositionAlloc->Add (Vector (592.76, 412.24, 35));
    enbPositionAlloc->Add (Vector (594.90, 190.89, 35));
    enbPositionAlloc->Add (Vector (38.84, 140.63, 35));
    enbPositionAlloc->Add (Vector (294.41, 55.08, 35));
  }else{
    enbPositionAlloc->Add (Vector (312.75,1356.41, 35));
    enbPositionAlloc->Add (Vector (10.80,1086.41, 35));
    enbPositionAlloc->Add (Vector (118.23,810.59, 35));
    enbPositionAlloc->Add (Vector (95.00,549.29, 35));
    enbPositionAlloc->Add (Vector (170.48,314.12, 35));
    enbPositionAlloc->Add (Vector (132.74,29.60, 35));
    enbPositionAlloc->Add (Vector (162.36,-312.41, 35));
  }
  
  
  
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (enbPositionAlloc);
  enbmobility.Install (enbNodes);


  Ptr<NrSlHelper> nrSlHelper = CreateObject <NrSlHelper> ();

  // NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice (enbNodes, allBwpsGnb);
  // use the bwp of sidelink
  // Config::SetDefault ("ns3::NrGnbPhy::Numerology", UintegerValue (numerologyBwpSl));
  
  nrHelper->SetGnbPhyAttribute ("Numerology", UintegerValue (numerologyBwpSl));
  nrHelper->SetGnbMacAttribute ("ActivePoolId", UintegerValue (0));
  nrSlHelper->SetEnbSlSchedulerAttribute ("ActivePoolId", UintegerValue (0));
  NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice (enbNodes, allBwps);

  // create building
  if (isUrbanScenario){
  // if (false){
    std::vector<std::vector<double>> buildingsPositionVector = readCSV(buildingsFile);
    for (int i = 0; i<(int) buildingsPositionVector.size(); ++i){
    // for (int i = 0; i<(int) 1; ++i){
      std::vector<double> buildPosVec = buildingsPositionVector[i];
      Ptr<Building> b = CreateObject<Building>();
      b->SetBoundaries(Box(buildPosVec[0], buildPosVec[1], 
                          buildPosVec[2], buildPosVec[3], 
                          0, 20));
      b->SetBuildingType(Building::Residential);
      b->SetExtWallsType(Building::ConcreteWithWindows);
    }

    BuildingsHelper::Install (enbNodes);
    BuildingsHelper::Install (ueVoiceContainer);  
  }
  

  for (auto nd = enbNetDev.Begin (); nd != enbNetDev.End (); ++nd){
		(*nd)->GetObject<NrGnbNetDevice> ()->GetPhy (0)->SetTxPower(-10);
    DynamicCast<NrGnbNetDevice> (*nd)->UpdateConfig ();
  }

  NetDeviceContainer ueVoiceNetDev = nrHelper->InstallUeDevice (ueVoiceContainer, allBwps);

  for (auto it = ueVoiceNetDev.Begin (); it != ueVoiceNetDev.End (); ++it)
  {
    // DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    Ptr<NrUeNetDevice> nruedev = DynamicCast<NrUeNetDevice> (*it);
    nruedev->UpdateConfig ();
    // std::cout << "Curr sfnsg gnb " << DynamicCast<NrGnbNetDevice> (enbNetDev.Get (0))->GetPhy(0)->GetCurrentSfnSf() <<
    // " sfnsf ue "<<  nruedev->GetPhy(0)->GetCurrentSfnSf() << " imsi " << nruedev->GetImsi();
  }

  
  // Put the pointers inside NrSlHelper
  nrSlHelper->SetEpcHelper (epcHelper);

  // attach ues to closest enb 
  nrSlHelper->AttachToClosestEnb(ueVoiceNetDev, enbNetDev);
  std::string errorModel = "ns3::NrEesmIrT1";
  nrSlHelper->SetSlErrorModel (errorModel);
  nrSlHelper->SetUeSlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel));

  // nrSlHelper->SetNrSlSchedulerTypeId (NrSlUeMacSchedulerSimple::GetTypeId());
  
  if(nrUeMacTypeNameComplete.compare(NrUeMac::GetTypeId().GetName())){
    nrSlHelper->SetNrSlSchedulerTypeId (NrSlUeMacSchedulerSimple::GetTypeId());
  }else{
    nrSlHelper->SetNrSlSchedulerTypeId(AiNrSlUeMacSchedulerSimple::GetTypeId());
  }
  
  nrSlHelper->SetUeSlSchedulerAttribute ("FixNrSlMcs", BooleanValue (true));
  nrSlHelper->SetUeSlSchedulerAttribute ("InitialNrSlMcs", UintegerValue (10));

  nrSlHelper->SetUeSlSchedulerAttribute ("PlmnId", StringValue(ltePlmnId));
  nrSlHelper->SetUeSlSchedulerAttribute ("REType", StringValue(reType));
  

  nrSlHelper->PrepareUeForSidelink (ueVoiceNetDev, bwpIdContainer);
  nrSlHelper->PrepareGnbForSidelink (enbNetDev, bwpIdContainer);

  /*
   * Start preparing for all the sub Structs/RRC Information Element (IEs)
   * of LteRrcSap::SidelinkPreconfigNr. This is the main structure, which would
   * hold all the pre-configuration related to Sidelink.
   */

  //SlResourcePoolNr IE
  LteRrcSap::SlResourcePoolNr slResourcePoolNr;
  //get it from pool factory
  Ptr<NrSlCommPreconfigResourcePoolFactory> ptrFactory = Create<NrSlCommPreconfigResourcePoolFactory> ();
  /*
   * Above pool factory is created to help the users of the simulator to create
   * a pool with valid default configuration. Please have a look at the
   * constructor of NrSlCommPreconfigResourcePoolFactory class.
   *
   * In the following, we show how one could change those default pool parameter
   * values as per the need.
   */
  // std::vector <std::bitset<1> > slBitmap = {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1};
  // std::vector <std::bitset<1> > slBitmap = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  // std::vector <std::bitset<1> > slBitmap = {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1};
  std::vector <std::bitset<1> > slBitmap = {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1};
  ptrFactory->SetSlTimeResources (slBitmap);
  ptrFactory->SetSlSensingWindow (100); // T0 in ms
  ptrFactory->SetSlSelectionWindow (5);
  ptrFactory->SetSlFreqResourcePscch (10); // PSCCH RBs
  ptrFactory->SetSlSubchannelSize (50);
  ptrFactory->SetSlMaxNumPerReserve (3);
  //Once parameters are configured, we can create the pool
  LteRrcSap::SlResourcePoolNr pool = ptrFactory->CreatePool ();
  slResourcePoolNr = pool;

  //Configure the SlResourcePoolConfigNr IE, which hold a pool and its id
  LteRrcSap::SlResourcePoolConfigNr slresoPoolConfigNr;
  slresoPoolConfigNr.haveSlResourcePoolConfigNr = true;
  //Pool id, ranges from 0 to 15
  uint16_t poolId = 0;
  LteRrcSap::SlResourcePoolIdNr slResourcePoolIdNr;
  slResourcePoolIdNr.id = poolId;
  slresoPoolConfigNr.slResourcePoolId = slResourcePoolIdNr;
  slresoPoolConfigNr.slResourcePool = slResourcePoolNr;

  //Configure the SlBwpPoolConfigCommonNr IE, which hold an array of pools
  LteRrcSap::SlBwpPoolConfigCommonNr slBwpPoolConfigCommonNr;
  //Array for pools, we insert the pool in the array as per its poolId
  slBwpPoolConfigCommonNr.slTxPoolSelectedNormal [slResourcePoolIdNr.id] = slresoPoolConfigNr;

  //Configure the BWP IE
  LteRrcSap::Bwp bwp;
  bwp.numerology = numerologyBwpSl;
  bwp.symbolsPerSlots = 14;
  bwp.rbPerRbg = 1;
  bwp.bandwidth = bandwidthBandSl;

  //Configure the SlBwpGeneric IE
  LteRrcSap::SlBwpGeneric slBwpGeneric;
  slBwpGeneric.bwp = bwp;
  slBwpGeneric.slLengthSymbols = LteRrcSap::GetSlLengthSymbolsEnum (14);
  slBwpGeneric.slStartSymbol = LteRrcSap::GetSlStartSymbolEnum (0);

  //Configure the SlBwpConfigCommonNr IE
  LteRrcSap::SlBwpConfigCommonNr slBwpConfigCommonNr;
  slBwpConfigCommonNr.haveSlBwpGeneric = true;
  slBwpConfigCommonNr.slBwpGeneric = slBwpGeneric;
  slBwpConfigCommonNr.haveSlBwpPoolConfigCommonNr = true;
  slBwpConfigCommonNr.slBwpPoolConfigCommonNr = slBwpPoolConfigCommonNr;

  //Configure the SlFreqConfigCommonNr IE, which hold the array to store
  //the configuration of all Sidelink BWP (s).
  LteRrcSap::SlFreqConfigCommonNr slFreConfigCommonNr;
  //Array for BWPs. Here we will iterate over the BWPs, which
  //we want to use for SL.
  for (const auto &it:bwpIdContainer)
    {
      // it is the BWP id
      slFreConfigCommonNr.slBwpList [it] = slBwpConfigCommonNr;
    }

  //Configure the TddUlDlConfigCommon IE
  LteRrcSap::TddUlDlConfigCommon tddUlDlConfigCommon;
  // tddUlDlConfigCommon.tddPattern = "DL|DL|DL|F|UL|UL|UL|UL|UL|UL|";
  tddUlDlConfigCommon.tddPattern = "DL|DL|F|UL|UL|UL|UL|UL|UL|UL|";
  //Configure the SlPreconfigGeneralNr IE
  LteRrcSap::SlPreconfigGeneralNr slPreconfigGeneralNr;
  slPreconfigGeneralNr.slTddConfig = tddUlDlConfigCommon;

  //Configure the SlUeSelectedConfig IE
  LteRrcSap::SlUeSelectedConfig slUeSelectedPreConfig;
  slUeSelectedPreConfig.slProbResourceKeep = 0;
  //Configure the SlPsschTxParameters IE
  LteRrcSap::SlPsschTxParameters psschParams;
  psschParams.slMaxTxTransNumPssch = 5;
  //Configure the SlPsschTxConfigList IE
  LteRrcSap::SlPsschTxConfigList pscchTxConfigList;
  pscchTxConfigList.slPsschTxParameters [0] = psschParams;
  slUeSelectedPreConfig.slPsschTxConfigList = pscchTxConfigList;

  /*
   * Finally, configure the SidelinkPreconfigNr This is the main structure
   * that needs to be communicated to NrSlUeRrc class
   */
  LteRrcSap::SidelinkPreconfigNr slPreConfigNr;
  slPreConfigNr.slPreconfigGeneral = slPreconfigGeneralNr;
  slPreConfigNr.slUeSelectedPreConfig = slUeSelectedPreConfig;
  slPreConfigNr.slPreconfigFreqInfoList [0] = slFreConfigCommonNr;

  //Communicate the above pre-configuration to the NrSlHelper
  nrSlHelper->InstallNrSlPreConfiguration (ueVoiceNetDev, slPreConfigNr);

  //Communicate the above pre-configuration to the NrSlHelper
  nrSlHelper->InstallNrSlEnbPreConfiguration (enbNetDev, slPreConfigNr);

  /****************************** End SL Configuration ***********************/

  /*
   * Fix the random streams
   */
  int64_t stream = 1;
  stream += nrHelper->AssignStreams (ueVoiceNetDev, stream);
  stream += nrSlHelper->AssignStreams (ueVoiceNetDev, stream);

  /*
   * Configure the IP stack, and activate NR Sidelink bearer (s) as per the
   * configured time.
   *
   * This example supports IPV4 and IPV6
   */

  InternetStackHelper internet;
  internet.Install (ueVoiceContainer);
  stream += internet.AssignStreams (ueVoiceContainer, stream);
  uint32_t dstL2Id = 255;
  Ipv4Address groupAddress4 ("225.0.0.0");     //use multicast address as destination
  Ipv6Address groupAddress6 ("ff0e::1");     //use multicast address as destination
  Address remoteAddress;
  Address localAddress;
  uint16_t port = 8000;
  Ptr<LteSlTft> tft;
  if (!useIPv6)
    {
      Ipv4InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv4Address (ueVoiceNetDev);

      // set the default gateway for the UE
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      for (uint32_t u = 0; u < ueVoiceContainer.GetN (); ++u)
        {
          Ptr<Node> ueNode = ueVoiceContainer.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
        }
      remoteAddress = InetSocketAddress (groupAddress4, port);
      localAddress = InetSocketAddress (Ipv4Address::GetAny (), port);
      tft = Create<LteSlTft> (LteSlTft::Direction::BIDIRECTIONAL, LteSlTft::CommType::GroupCast, groupAddress4, dstL2Id);
      //Set Sidelink bearers
      // nrSlHelper->ActivateNrSlBearer (finalSlBearersActivationTime, ueVoiceNetDev, tft);
      finalSlBearersActivationTime = Seconds(0);
      nrSlHelper->ActivateNrSlBearerAllBwps (finalSlBearersActivationTime, ueVoiceNetDev, tft, bwpIdContainer);
    }
  else
    {
      Ipv6InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv6Address (ueVoiceNetDev);

      // set the default gateway for the UE
      Ipv6StaticRoutingHelper ipv6RoutingHelper;
      for (uint32_t u = 0; u < ueVoiceContainer.GetN (); ++u)
        {
          Ptr<Node> ueNode = ueVoiceContainer.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv6StaticRouting> ueStaticRouting = ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
        }
      remoteAddress = Inet6SocketAddress (groupAddress6, port);
      localAddress = Inet6SocketAddress (Ipv6Address::GetAny (), port);
      tft = Create<LteSlTft> (LteSlTft::Direction::BIDIRECTIONAL, LteSlTft::CommType::GroupCast, groupAddress6, dstL2Id);
      //Set Sidelink bearers
      // nrSlHelper->ActivateNrSlBearer (finalSlBearersActivationTime, ueVoiceNetDev, tft);
      nrSlHelper->ActivateNrSlBearerAllBwps (finalSlBearersActivationTime, ueVoiceNetDev, tft, bwpIdContainer);
    }


  //Set Application in the UEs
  OnOffHelper sidelinkClient ("ns3::UdpSocketFactory", remoteAddress);
  sidelinkClient.SetAttribute ("EnableSeqTsSizeHeader", BooleanValue (true));
  std::string dataRateBeString  = std::to_string (dataRateBe) + "kb/s";
  std::cout << "Data rate " << DataRate (dataRateBeString) << std::endl;
  sidelinkClient.SetConstantRate (DataRate (dataRateBeString), udpPacketSizeBe);

  // install odd index of ue
  ApplicationContainer clientApps;
  // for (uint32_t _ind = 0; _ind<1; _ind+=2){
  for (uint32_t _ind = 0; _ind<ueVoiceContainer.GetN(); _ind+=2){
    clientApps.Add(sidelinkClient.Install (ueVoiceContainer.Get (_ind)));
  }
  
  clientApps.Start (Seconds(finalSlBearersActivationTime.GetSeconds()));
  clientApps.Stop (finalSimTime);

  //Output app start, stop and duration
  double realAppStart =  finalSlBearersActivationTime.GetSeconds () + ((double)udpPacketSizeBe * 8.0 / (DataRate (dataRateBeString).GetBitRate ()));
  double appStopTime = (finalSimTime).GetSeconds ();

  std::cout << "App start time " << realAppStart << " sec" << std::endl;
  std::cout << "App stop time " << appStopTime << " sec" << std::endl;


  ApplicationContainer serverApps;
  PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory", localAddress);
  sidelinkSink.SetAttribute ("EnableSeqTsSizeHeader", BooleanValue (true));
  for (uint32_t _ind = 0; _ind<ueVoiceContainer.GetN(); ++_ind){
  // for (uint32_t _ind = 0; _ind<ueVoiceContainer.GetN(); ++_ind){
    serverApps.Add(sidelinkSink.Install (ueVoiceContainer.Get (_ind)));
  }
  // serverApps = sidelinkSink.Install (ueVoiceContainer.Get (ueVoiceContainer.GetN () - 1));
  serverApps.Start (Seconds (0));

  // modified

  // nrHelper->EnableTraces();

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/SensingData",
                                   MakeBoundCallback (&NrGnbNetDevice::SciReceptionCallback, DynamicCast<NrGnbNetDevice> (enbNetDev.Get (0))));

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUeMac/E2PacketDelayReport",
                                   MakeBoundCallback (&NrGnbNetDevice::PacketDelaysInBufferCallback, DynamicCast<NrGnbNetDevice> (enbNetDev.Get (0))));
  
  
  // connect the trace coming from the gnb to the ue mac
  

  for (auto it = ueVoiceNetDev.Begin (); it != ueVoiceNetDev.End (); ++it)
    {
      Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/SlV2XScheduling",
                                   MakeBoundCallback (&NrUeMac::V2xE2SchedulingTraceCallback, DynamicCast<NrUeMac> (DynamicCast<NrUeNetDevice> (*it)->GetMac (0))));
    }

  // store the received scheduling command to sql to compare with the ones sent
  
  // end modification

  //Trace receptions; use the following to be robust to node ID changes
  std::ostringstream path;
  path << "/NodeList/" << ueVoiceContainer.Get (1)->GetId () << "/ApplicationList/0/$ns3::PacketSink/Rx";
  Config::ConnectWithoutContext(path.str (), MakeCallback (&ReceivePacket));
  path.str ("");

  path << "/NodeList/" << ueVoiceContainer.Get (1)->GetId () << "/ApplicationList/0/$ns3::PacketSink/Rx";
  Config::ConnectWithoutContext(path.str (), MakeCallback (&ComputePir));
  path.str ("");

  path << "/NodeList/" << ueVoiceContainer.Get (0)->GetId () << "/ApplicationList/0/$ns3::OnOffApplication/Tx";
  Config::ConnectWithoutContext(path.str (), MakeCallback (&TransmitPacket));
  path.str ("");

  Ptr<SlStatsHelper> slStatsHelperPtr = CreateObject<SlStatsHelper>();
  slStatsHelperPtr->SetAttribute("TracesPath", StringValue(outputDir + simTag + "/"));
  // slStatsHelperPtr->SetUeRlcBufferSizeFilename(outputDir + simTag + "/" + "UeRlcBufferSizeStats.txt");
  // slStatsHelperPtr->SetUeRlcRxFilename(outputDir + simTag + "/" + "UeRlcRxStats.txt");
  // slStatsHelperPtr->SetUeRlcTxFilename(outputDir + simTag + "/" + "UeRlcTxStats.txt");
  // slStatsHelperPtr->CreateFileRclBufferSize();
  // slStatsHelperPtr->CreateFileRlcRxSize();
  // slStatsHelperPtr->CreateFileRlcTxSize();

  //Datebase setup
  std::string exampleName = simTag + "/" + "nr-v2x-simple-demo.db";
  SQLiteOutput db (outputDir + exampleName);

  uint32_t writeCacheSize = 2500 * ueNum; 
  // uint32_t writeCacheSize = 10;

  // modified
  UeV2XScheduling v2xSchedulingxApp;
  v2xSchedulingxApp.SetDb (&db, "v2XSchedulingXapp", writeCacheSize); 
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/SlV2XScheduling",
                                   MakeBoundCallback (&SlStatsHelper::NotifyxAppScheduling, &v2xSchedulingxApp, DynamicCast<NrGnbNetDevice> (enbNetDev.Get (0))->GetPhy(0)));

  // Mode 2 scheduling
  UeV2XScheduling v2xSchedulingMode2;
  v2xSchedulingMode2.SetDb (&db, "v2xSchedulingMode2", writeCacheSize); 
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUeMac/UeSlV2XScheduling",
                                   MakeBoundCallback (&SlStatsHelper::NotifyUeScheduling, &v2xSchedulingMode2));

  // writeCacheSize = 2500 * ueNum; 
  // mobility 
  // control messages
  // gnbPhy->TraceConnectWithoutContext ("SlotDataStats",
  // 								MakeBoundCallback (&SlStatsHelper::ReportSlotStatsNr, &slotStats));		
  // gnbPhy->TraceConnectWithoutContext ("SlotCtrlStats",
  // 										MakeBoundCallback (&SlStatsHelper::ReportSlotCtrlStatsNr, &slotCtrlStats));



  SlSinrOutputStats sinrStats;
	SlPowerOutputStats ueTxPowerStats;
	SlSlotOutputStats slotStats;
	SlSlotCtrlOutputStats slotCtrlStats;
	// SlPhySlotStats phySlotStats;
	// SlRbOutputStats rbStats;
	// SlPowerOutputStats gnbRxPowerStats;
	SlDownlinkTbSizeStats ueDLTbSizeStats;
	SlUplinkTbSizeStats ueULTbSizeStats;
	SlFlowMonitorOutputStats flowMonStats;
	SlPacketTraceStats packetTraceStats;
	SlMacSRStats macSRStats;
	// SlMacBsrStats macBsrStats; 
	SlSchedulingStats dlScheduling;
  SlSchedulingStats ulScheduling;
	// bsr stats coming from ue
	SlMacUeBsrStats macUeBsrStats;
	// ul dci ctrl
	SlCtrlUlDciStats ctrlUlDciStats;
  // phy and mac ctrl msgs
  SlCtrlMsgsStats ctrlMsgsStats;
  // mobility 
  SlMobilityStats mobilityStats;
	sinrStats.SetDb (&db, "ueSINR", writeCacheSize);
	ueTxPowerStats.SetDb (&db, "ueTxPower", writeCacheSize);
	slotStats.SetDb (&db, "slotStats", writeCacheSize);
	slotCtrlStats.SetDb (&db, "slotCtrlStats", writeCacheSize);
	flowMonStats.SetDb (&db, "flows", writeCacheSize);
	ueULTbSizeStats.SetDb(&db, "ulTbSize", writeCacheSize);
	ueDLTbSizeStats.SetDb(&db, "dlTbSize", writeCacheSize);
	packetTraceStats.SetDb(&db, "packetTrace", writeCacheSize);
	// MAC
	macSRStats.SetDb (&db, "macSR", writeCacheSize);
	dlScheduling.SetDb(&db, "dlScheduling", writeCacheSize);
  ulScheduling.SetDb(&db, "ulScheduling", writeCacheSize);
	macUeBsrStats.SetDb(&db, "macUeBsr", writeCacheSize);
	ctrlUlDciStats.SetDb(&db, "ctrlUlDciStats", writeCacheSize);
  ctrlMsgsStats.SetDb(&db, "ctrlMsgsStats", writeCacheSize);
  mobilityStats.SetDb(&db, "mobility", writeCacheSize);

  



	// macBsrStats.SetDb(&db, "macBsr", 100);
	// phySlotStats.SetDb (&db, "phySlotStats");
	// rbStats.SetDb (&db);
	// gnbRxPowerStats.SetDb (&db, "gnbRxPower");
  
  // GNB PHY
  // Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbPhy/PhySlotStats",
  //                 MakeBoundCallback (&SlStatsHelper::ReportSlPhySlotStats, &phySlotStats));
  // Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbPhy/RBDataStats",
  //                 MakeBoundCallback (&SlStatsHelper::ReportRbStatsNr, &rbStats));
  // the ul dci ctrl
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbPhy/UlDciStats",
                  MakeBoundCallback (&SlStatsHelper::ReportCtrlUlDci, &ctrlUlDciStats));
  // Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbPhy/NrSpectrumPhyList/*/RxDataTrace",
  //                 MakeBoundCallback (&SlStatsHelper::ReportGnbRxDataNr, &gnbRxPowerStats));
  // // activate the traces showing the packet received by ue and enodebs
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbPhy/NrSpectrumPhyList/*/RxPacketTraceEnb",
                  MakeBoundCallback (&SlStatsHelper::ReportPacketTrace, &packetTraceStats, "UL"));

  // GNB MAC
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbMac/SrReq",
				MakeBoundCallback (&SlStatsHelper::ReportMacSR, &macSRStats));
  // activating traces on bsr coming from rlc
  // Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbMac/RlcBufferStatus",
	// 			MakeBoundCallback (&SlStatsHelper::ReportMacBsr, &macBsrStats));
		// activating traces on ue received bsr
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbMac/UeBsr",
				MakeBoundCallback (&SlStatsHelper::ReportUeMacBsr, &macUeBsrStats));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbMac/DlScheduling",
				MakeBoundCallback (&SlStatsHelper::ReportSlMacScheduling, &dlScheduling));
  
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbMac/UlScheduling",
				MakeBoundCallback (&SlStatsHelper::ReportSlMacScheduling, &ulScheduling));

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbPhy/GnbPhyRxedCtrlMsgsTrace",
											   MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "PHY", "Gnb Rxed"));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbPhy/GnbPhyTxedCtrlMsgsTrace",
											   MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "PHY", "Gnb Txed"));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbMac/GnbMacRxedCtrlMsgsTrace",
											   MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "MAC", "Gnb Rxed"));
	Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/BandwidthPartMap/*/NrGnbMac/GnbMacTxedCtrlMsgsTrace",
											   MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "MAC", "Gnb Txed"));

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUeMac/UeMacRxedCtrlMsgsTrace",
												   MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "MAC", "Ue Rxed"));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUeMac/UeMacTxedCtrlMsgsTrace",
												   MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "MAC", "Ue Txed"));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/UePhyRxedCtrlMsgsTrace",
													MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "PHY", "Ue Rxed"));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/UePhyTxedCtrlMsgsTrace",
                          MakeBoundCallback(&SlStatsHelper::ReportCtrlMessages, &ctrlMsgsStats, "PHY", "Ue Txed"));

  // ue phy 
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/NrSpectrumPhyList/*/RxPacketTraceUe",
                          MakeBoundCallback(&SlStatsHelper::ReportPacketTrace, &packetTraceStats, "DL"));
  for (auto nd = enbNetDev.Begin (); nd != enbNetDev.End (); ++nd){
		Ptr<LteEnbRrc> gnbRrc = (*nd)->GetObject<NrGnbNetDevice> ()->GetRrc ();
    gnbRrc->TraceConnectWithoutContext("NewUeContext", 
                          MakeBoundCallback (&NewUeContext, gnbRrc, outputDir+simTag+"/" ));
  }

  // ue rrc state transition
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/LteUeRrc/StateTransition",
                                  MakeBoundCallback (&SwitchStateUe, outputDir+ simTag+"/" ));

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/ReportPowerSpectralDensity",
											MakeBoundCallback (&SlStatsHelper::ReportPowerNr, &ueTxPowerStats));
  // end modification

  // writeCacheSize = 100;

  NS_LOG_DEBUG("WriteCAche " << writeCacheSize);

  UeMacPscchTxOutputStats pscchStats;
  pscchStats.SetDb (&db, "pscchTxUeMac", writeCacheSize);
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUeMac/SlPscchScheduling",
                                   MakeBoundCallback (&SlStatsHelper::NotifySlPscchScheduling, &pscchStats));

  UeMacPsschTxOutputStats psschStats;
  psschStats.SetDb (&db, "psschTxUeMac", writeCacheSize);
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUeMac/SlPsschScheduling",
                                   MakeBoundCallback (&SlStatsHelper::NotifySlPsschScheduling, &psschStats));


  UePhyPscchRxOutputStats pscchPhyStats;
  pscchPhyStats.SetDb (&db, "pscchRxUePhy", writeCacheSize);
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/NrSpectrumPhyList/*/RxPscchTraceUe",
                                   MakeBoundCallback (&SlStatsHelper::NotifySlPscchRx, &pscchPhyStats));

  UePhyPsschRxOutputStats psschPhyStats;
  psschPhyStats.SetDb (&db, "psschRxUePhy", writeCacheSize);
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/NrSpectrumPhyList/*/RxPsschTraceUe",
                                   MakeBoundCallback (&SlStatsHelper::NotifySlPsschRx, &psschPhyStats));

  UeToUePktTxRxOutputStats pktStats;
  pktStats.SetDb (&db, "pktTxRx", writeCacheSize);

  SlSpectrumStats slSpectrumStats;
  slSpectrumStats.SetDb (&db, "slSpectrum", writeCacheSize);
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/NrSpectrumPhyList/*/SlRxFrameCtrlTrace",
                          MakeBoundCallback(&SlStatsHelper::ReportSlSpectrum, &slSpectrumStats, "RX", "CTRL"));

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/NrSpectrumPhyList/*/SlRxFrameDataTrace",
                          MakeBoundCallback(&SlStatsHelper::ReportSlSpectrum, &slSpectrumStats, "RX", "DATA"));
  
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/NrSpectrumPhyList/*/SlTxFrameCtrlTrace",
                          MakeBoundCallback(&SlStatsHelper::ReportSlSpectrum, &slSpectrumStats, "TX", "CTRL"));

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/NrSpectrumPhyList/*/SlTxFrameDataTrace",
                          MakeBoundCallback(&SlStatsHelper::ReportSlSpectrum, &slSpectrumStats, "TX", "DATA"));

  if (!useIPv6)
    {
      // Set Tx traces
      for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
        {
          Ipv4Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Tx address: " << localAddrs << std::endl;
          // clientApps.Get (ac)->TraceConnect ("TxWithSeqTsSize", "tx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, ueVoiceContainer.Get (0), localAddrs));
          clientApps.Get (ac)->TraceConnect ("TxWithSeqTsSize", "tx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, clientApps.Get (ac)->GetNode (), localAddrs));
        }

      // Set Rx traces
      for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
        {
          Ipv4Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Rx address: " << localAddrs << std::endl;
          
          // serverApps.Get (ac)->TraceConnect ("RxWithSeqTsSize", "rx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, ueVoiceContainer.Get (1), localAddrs));
          serverApps.Get (ac)->TraceConnect ("RxWithSeqTsSize", "rx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, serverApps.Get (ac)->GetNode (), localAddrs));
        }
    }
  else
    {
      // Set Tx traces
      for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
        {
          clientApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->AddMulticastAddress (groupAddress6);
          Ipv6Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->GetAddress (1,1).GetAddress ();
          std::cout << "Tx address: " << localAddrs << std::endl;
          // clientApps.Get (ac)->TraceConnect ("TxWithSeqTsSize", "tx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, ueVoiceContainer.Get (0), localAddrs));
          clientApps.Get (ac)->TraceConnect ("TxWithSeqTsSize", "tx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, clientApps.Get (ac)->GetNode (), localAddrs));
        }

      // Set Rx traces
      for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
        {
          serverApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->AddMulticastAddress (groupAddress6);
          Ipv6Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->GetAddress (1,1).GetAddress ();
          std::cout << "Rx address: " << localAddrs << std::endl;
          // serverApps.Get (ac)->TraceConnect ("RxWithSeqTsSize", "rx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, ueVoiceContainer.Get (ac), localAddrs));
          serverApps.Get (ac)->TraceConnect ("RxWithSeqTsSize", "rx", MakeBoundCallback (&SlStatsHelper::UePacketTraceDb, &pktStats, serverApps.Get (ac)->GetNode (), localAddrs));
        }
    }

  ns2.Install();

  PrintGnuplottableEnbListToFile(outputDir+simTag+"/"+"gnbPos.txt");
  PrintGnuplottableUeListToFile(outputDir+simTag+"/"+"uePos.txt");
  PrintGnuplottableBuildingListToFile(outputDir+simTag+"/"+"buildings.txt");
  std::string nodePositionFilename = outputDir+simTag+"/"+"node_position.txt";
  std::ofstream outFile {};
  outFile.open(nodePositionFilename.c_str(), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
  {
		NS_FATAL_ERROR ("Can't open file " << nodePositionFilename.c_str ());
	}else{
    outFile << "time" << "," << "imsi" << "," << "nodeid" << "," << "x" << "," << "y" << "," << "z" << "\n";
  }
  outFile.close();
  for (uint32_t time = 100; time < finalSimTime.GetMilliSeconds(); time+=10){
    Simulator::Schedule(MilliSeconds(time), &PrintPosition, ueVoiceNetDev, nodePositionFilename);
  }

  for (auto it = ueVoiceNetDev.Begin (); it != ueVoiceNetDev.End (); ++it)
  {
    Ptr<NrUeNetDevice> dev = DynamicCast<NrUeNetDevice> (*it);

    Ptr<MobilityModel> mm = dev->GetNode()->GetObject<MobilityModel> ();
    mm->TraceConnectWithoutContext("CourseChange", MakeBoundCallback (&SlStatsHelper::CourseChange, &mobilityStats, (*it), "ue"));
  }

  for (auto it = enbNetDev.Begin (); it != enbNetDev.End (); ++it)
  {
    Ptr<NrGnbNetDevice> dev = DynamicCast<NrGnbNetDevice> (*it);

    Ptr<MobilityModel> mm = dev->GetNode()->GetObject<MobilityModel> ();
    mm->TraceConnectWithoutContext("CourseChange", MakeBoundCallback (&SlStatsHelper::CourseChange, &mobilityStats, (*it), "gnb"));
  }

  // Config::ConnectWithoutContext (
  //   "ns3::LteRlc::BufferSizeTracedCallback",
    // "/NodeList/*/DeviceList/*/LteUeRrc/DataRadioBearerMap/*/LteRlc/BufferSize",
                      // MakeBoundCallback (&SlStatsHelper::RxRlcBufferSizeSl, slStatsHelperPtr->GetUeRlcBufferSizeFilename()));

  Simulator::Stop (finalSimTime);
  Simulator::Run ();

  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  std::cout << "Total Tx bits = " << txByteCounter * 8 << std:: endl;
  std::cout << "Total Tx packets = " << txPktCounter << std:: endl;

  std::cout << "Total Rx bits = " << rxByteCounter * 8 << std:: endl;
  std::cout << "Total Rx packets = " << rxPktCounter << std:: endl;

  std::cout << "Avrg thput = " << (rxByteCounter * 8) / (finalSimTime - Seconds(realAppStart)).GetSeconds () / 1000.0 << " kbps" << std:: endl;

  std::cout << "Average Packet Inter-Reception (PIR) " << pir.GetSeconds () / pirCounter << " sec" << std::endl;



  /*
   * VERY IMPORTANT: Do not forget to empty the database cache, which would
   * dump the data store towards the end of the simulation in to a database.
   */
  pktStats.EmptyCache ();
  pscchStats.EmptyCache ();
  psschStats.EmptyCache ();
  pscchPhyStats.EmptyCache ();
  psschPhyStats.EmptyCache ();
  v2xSchedulingxApp.EmptyCache ();

  ueTxPowerStats.EmptyCache();
  dlScheduling.EmptyCache();
  ulScheduling.EmptyCache();
  ctrlMsgsStats.EmptyCache();
  macSRStats.EmptyCache();
  macUeBsrStats.EmptyCache();
  ctrlUlDciStats.EmptyCache();
  packetTraceStats.EmptyCache();
  mobilityStats.EmptyCache();

  Simulator::Destroy ();
  return 0;
}


