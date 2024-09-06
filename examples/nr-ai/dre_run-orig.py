# Copyright (c) 2021-2023 Huazhong University of Science and Technology
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Xun Deng <dorence@hust.edu.cn>
#         Hao Yin <haoyin@uw.edu>
#         Muyuan Shen <muyuan_shen@hust.edu.cn>

import os
import sys
import ns3ai_ore_py_stru as py_binding
# sys.path.append( os.path.join(os.getcwd(), '../../python_utils/'))
from ns3ai_utils import Experiment
import traceback
import csv
import math
import logging
from datetime import datetime
import argparse

def Sort(sub_li,n):
    sub_li.sort(key = lambda x: x[n],reverse=True)
    return sub_li

class ORE:
    def __init__(self, ns3_exec_name: str, ns3_dir: str, output_dir: str = "./", ns3_settings={}, ns3_log_filename= "logfile.log",
                 ns3_ai_log_filename = "ai_log.log") -> None:
        self.output_dir = output_dir
        self.ns3_exec_name = ns3_exec_name
        self.ns3_dir = ns3_dir
        self.ns3_settings = ns3_settings
        self.ns3_ai_log_filename= os.path.join(output_dir, ns3_ai_log_filename)
        self.ns3_log_file = open(os.path.join(output_dir, ns3_log_filename), 'w')
        self.NExGammaX2NSe1 = []
        self.NExGammaX2NSe2 = []
        self.NExGammaX2NSe3 = []
        self.NExGammaX2NSe4 = []
        self.NEx = []
        self.settingsChoicesRhoUEp1 = []
        self.settingsChoicesRhoUEp2 = []
        self.settingsChoicesRhoUEp4 = []
        self.numUe = 300
        self.alpha = 1
        self.lastNumExclusions = [1000]*self.numUe
        self.lastNumExclusionsCsv = []
        self.lastRsrpThreshold = [0]*self.numUe
        self.lastRsrpThresholdCsv = []
        self.currentRhoUeEstimate = [.1]*self.numUe
        self.currentRhoUeEstimateCsv = []
        self.currentNSe = [1]*self.numUe
        self.lastImsi = -1#this needs to be defined because at the very begining of the simulation selectionModeSelection is called before estimatoreUpdate. This isn't and isue because it gets quickly overwritten
        #and affects nothing
        
        self.dV = 10
        self.numLanes = 2
        

        self.initialize()

    def initialize(self):
        logger = logging.getLogger("")

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/dre" ,'NEx+gammaX2rhoUE,NSe1,T2T132,RRI100.csv') ) as csvFile:#all the values of gammaX and NEx are identical for all NSe so theres no need to get them after the first file read
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    gammaX = [float(x) for x in row[1:]]
                    lineCount += 1
                else:
                    self.NEx.append(int(row[0]))
                    self.NExGammaX2NSe1.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/dre" ,'NEx+gammaX2rhoUE,NSe2,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    lineCount += 1
                else:
                    self.NExGammaX2NSe2.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/dre" ,'NEx+gammaX2rhoUE,NSe3,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    lineCount += 1
                else:
                    self.NExGammaX2NSe3.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/dre" ,'NEx+gammaX2rhoUE,NSe4,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    lineCount += 1
                else:
                    self.NExGammaX2NSe4.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/dre" ,'settingsTable,rhoUE,rhoUEp1,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            for row in csvReader:
                self.settingsChoicesRhoUEp1.append([float(x) for x in row[0:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/dre" ,'settingsTable,rhoUE,rhoUEp2,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            for row in csvReader:
                self.settingsChoicesRhoUEp2.append([float(x) for x in row[0:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/dre" ,'settingsTable,rhoUE,rhoUEp4,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            for row in csvReader:
                self.settingsChoicesRhoUEp4.append([float(x) for x in row[0:]])


    def get_action(self, env):
        logger = logging.getLogger("")
        logger.info(f"DRE get action, selectionModeSelectionFlag {env.selectionModeSelectionFlag}" )
        # with open(os.path.join(self.output_dir, 'encodedSelectionInstructions.txt'), 'w') as file:
        #     file.write(str(2) + str(1))
        # return float(str(2) + str(1))
        
        if env.sensingDataFlag == 1:
            self.lastImsi = env.imsi-1-math.floor(env.imsi/256)#this is saved because the functions are called in sequence and NrSlUeMacSchedulerSimple does not have access to imsi. any calls made by NrSlUeMacSchedulerSimple will be related to this imsi though
            self.lastNumExclusions[self.lastImsi] = env.occupiedResources
            self.lastRsrpThreshold[self.lastImsi] = env.rsrpThreshold + 110 - 3
        else:
            if self.lastImsi > -1:#in the first few milliseconds this can be called before any packets are recieved, it doesnt do anything so we just give it dummy values
                logger.info("Finding resources ")
                logger.info(f"Time {env.time}")

                ########################################################
                #estimating rhoUE
                if (self.lastImsi > (150/self.dV - 1) and self.lastImsi < (750/self.dV - 150/self.dV - 1)) or (self.lastImsi > (900/self.dV - 1) and self.lastImsi < (1500/self.dV - 150/self.dV - 1)):
                    #temp = [abs(x - lastRsrpThreshold[self.lastImsi]) for x in gammaX]
                    #closestGammaX = int(temp.index(min(temp)))
                    if self.currentNSe[self.lastImsi] == 1:#we keep self.currentNSe for rhoUE estimation even if it isn't used because selection modde 1 is selected, it is selected later as the last entry of the settings table
                        self.currentRhoUeEstimate[self.lastImsi] = self.alpha*self.NExGammaX2NSe1[self.lastNumExclusions[self.lastImsi]][int(self.lastRsrpThreshold[self.lastImsi]/3)] + (1-self.alpha)*self.currentRhoUeEstimate[self.lastImsi]
                    elif self.currentNSe[self.lastImsi] == 2:
                        self.currentRhoUeEstimate[self.lastImsi] = self.alpha*self.NExGammaX2NSe2[self.lastNumExclusions[self.lastImsi]][int(self.lastRsrpThreshold[self.lastImsi]/3)] + (1-self.alpha)*self.currentRhoUeEstimate[self.lastImsi]
                    elif self.currentNSe[self.lastImsi] == 3:
                        self.currentRhoUeEstimate[self.lastImsi] = self.alpha*self.NExGammaX2NSe3[self.lastNumExclusions[self.lastImsi]][int(self.lastRsrpThreshold[self.lastImsi]/3)] + (1-self.alpha)*self.currentRhoUeEstimate[self.lastImsi]
                    elif self.currentNSe[self.lastImsi] == 4:
                        self.currentRhoUeEstimate[self.lastImsi] = self.alpha*self.NExGammaX2NSe4[self.lastNumExclusions[self.lastImsi]][int(self.lastRsrpThreshold[self.lastImsi]/3)] + (1-self.alpha)*self.currentRhoUeEstimate[self.lastImsi]
                else:#force edge UE to be correct
                    self.currentRhoUeEstimate[self.lastImsi] = self.numLanes/self.dV

                ########################################################
                #recording data
                if self.lastNumExclusions[self.lastImsi] > 0 and self.lastImsi == 75:
                    self.currentRhoUeEstimateCsv.append([env.time] + self.currentRhoUeEstimate)
                    self.lastNumExclusionsCsv.append([env.time] + self.lastNumExclusions)
                    self.lastRsrpThresholdCsv.append([env.time] + self.lastRsrpThreshold)
                logger.info(self.currentRhoUeEstimate[self.lastImsi])
                
                ########################################################
                #setting new NSe
                if self.currentRhoUeEstimate[self.lastImsi] <= .15:
                    NSe = int(self.settingsChoicesRhoUEp1[int(len(self.settingsChoicesRhoUEp1)-1)][2])
                    self.currentNSe[self.lastImsi] = NSe
                elif self.currentRhoUeEstimate[self.lastImsi] > .15 and self.currentRhoUeEstimate[self.lastImsi] <= .3:
                    NSe = int(self.settingsChoicesRhoUEp2[int(len(self.settingsChoicesRhoUEp2)-1)][2])
                    self.currentNSe[self.lastImsi] = NSe
                elif self.currentRhoUeEstimate[self.lastImsi] > .3:
                    NSe = int(self.settingsChoicesRhoUEp4[int(len(self.settingsChoicesRhoUEp4)-1)][2])
                    self.currentNSe[self.lastImsi] = NSe
                encodedSelectionInstructions = float(str(2) + str(NSe))
                with open(os.path.join(self.output_dir, 'encodedSelectionInstructions.txt'), 'w') as file:
                    file.write(str(2) + str(NSe))
                return encodedSelectionInstructions

            else:#this condition is only triggered in the first few miliseconds, after the initial selection and before any reselection happen. It will be overwritten by the first reselection.
                logger.info("pear")
                #print(float(str(2) + str(1)))
                encodedSelectionInstructions = float(str(2) + str(1))
                with open(os.path.join(self.output_dir, 'encodedSelectionInstructions.txt'), 'w') as file:
                    file.write(str(2) + str(1))
                return encodedSelectionInstructions

    def run(self):
        logger = logging.getLogger("")
        # first parameter is the ns3 script to run
        _n_re_type = "reType"
        reType = self.ns3_settings[_n_re_type]
        _n_e2_ltePlmnId = 'ltePlmnId'
        plmnId = self.ns3_settings[_n_e2_ltePlmnId]
        logger.info(f"plmnd id {plmnId} & reType {reType}")
        exp = Experiment(self.ns3_exec_name, self.ns3_dir, py_binding, handleFinish=True,
                         segName=f"Seg{reType}{plmnId}",
                         cpp2pyMsgName=f"CppToPythonMsg{reType}{plmnId}",
                         py2cppMsgName=f"PythonToCppMsg{reType}{plmnId}",
                         lockableName=f"Lockable{reType}{plmnId}",
                         )
        msgInterface = exp.run(setting=self.ns3_settings, show_output=True, log_file= self.ns3_log_file)

        try:
            while True:
                logger.info("DRE run")
                msgInterface.PyRecvBegin()
                msgInterface.PySendBegin()
                if msgInterface.PyGetFinished():
                    break

                msgInterface.GetPy2CppStruct().encodedSelectionInstructions = (
                    self.get_action(msgInterface.GetCpp2PyStruct()))
            
                msgInterface.PyRecvEnd()
                msgInterface.PySendEnd()

        except Exception as e:
            
            exc_type, exc_value, exc_traceback = sys.exc_info()
            logger.info("Exception occurred: {}".format(e))
            logger.info("Traceback:")
            logger.error("Logging an uncaught exception",
                 exc_info=(exc_type, exc_value, exc_traceback))
            # writing the finish time to the read me file
            _v_sim_stop_time = datetime.now()
            # Add to read me file
            _t_readme_text_file = open(os.path.join(self.output_dir, "README.txt"), "a")
            _t_readme_text_file.write("\n\nSimulation ending time: " + str(_v_sim_stop_time))
            _t_readme_text_file.close()
            # traceback.print_tb(exc_traceback)
            exit(1)

        else:
            pass

        finally:
            logger.info("Finally exiting...")
            del exp


def setup_logger(filename_full_path):
    logging.basicConfig(level=logging.INFO, filename=filename_full_path, filemode='a',
                        format='%(asctime)-15s %(levelname)-8s %(message)s')
    
    logger = logging.getLogger('')
    formatter = logging.Formatter('%(asctime)-15s %(levelname)-8s %(message)s')
    console = logging.StreamHandler()
    console.setLevel(logging.INFO)
    console.setFormatter(formatter)
    logger.addHandler(console)

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("--execFilename", help="the ns3 filename to execute",
                type=str)
    parser.add_argument("--outputDirectory", help="the directory to output the ns3 simulation log",
                type=str)
    parser.add_argument("--simulationDirectory", help="the directory where the ns3 is located",
                type=str)
    parser.add_argument("--ns3LogFileName", help="the filename to output the ns3 simulation log",
                type=str)
    parser.add_argument("--aiLogFileName", help="the filename to output the ai/python simulation log",
                type=str)
    parser.add_argument("--ns3SettingsStr", help="the ns3 parameters in string",
                type=str)
    args = parser.parse_args()

    ns3_ai_log_filename_full_path = os.path.join(args.outputDirectory, args.ns3LogFileName)

    setup_logger(ns3_ai_log_filename_full_path)

    # logger = logging.getLogger("")

    # logger.info("Parsed args")
    # logger.info(args)

    _ore = ORE( args.execFilename, args.simulationDirectory, 
                output_dir = args.outputDirectory, 
                ns3_log_filename= args.ns3LogFileName,
                ns3_ai_log_filename = args.aiLogFileName,
                ns3_settings = eval(args.ns3SettingsStr),
            )
    

    _ore.run()



