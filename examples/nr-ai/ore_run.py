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


import ns3ai_ore_py_stru as py_binding
from ns3ai_utils import Experiment
import sys
import traceback
import csv
import math
import os
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
        self.numUe = 300
        self.alpha = .5
        self.beta = .5
        self.gamma = .1
        self.lastNumExclusions = [1000]*self.numUe
        self.lastNumExclusionsCsv = []
        self.lastRsrpThreshold = [0]*self.numUe
        self.lastRsrpThresholdCsv = []
        self.currentRhoUeEstimate = [.1]*self.numUe
        self.currentRhoUeEstimateCsv = []
        self.currentNSe = [1]*self.numUe
        self.lastImsi = -1#this needs to be defined because at the very begining of the simulation selectionModeSelection is called before estimatoreUpdate. This isn't and isue because it gets quickly overwritten
        #and affects nothing
        self.settingsChoicesRhoUEp1 = []
        self.settingsChoicesRhoUEp2 = []
        self.settingsChoicesRhoUEp4 = []

        self.currentDEdgeEstimate = [75]*self.numUe
        self.currentDEdgeEstimateCsv = []
        self.currentNSe = [1]*self.numUe
        self.queryFlag = [0]*self.numUe
        self.selectionModeSelectionCounter = 0
        self.OReCounter = 0
        
        self.lastRecievedPackets = [ [] for _ in range(self.numUe) ]

        self.rhoUE = []
        self.bestMode = []

        self.numLanes = 2
        self.numUePerLane = int(self.numUe/self.numLanes)#this should always be an integer
        self.dV = 10
        self.dL = 4
        self.uePos = []
        self.initialPos = [0,0]

        self.dZ = 5
        self.zoneId = []
        self.zoneCenters = []
        self.zoneStart = [-2.5,-3]

        self.t1 = 2

        self.initialize()

    def initialize(self):
        logger = logging.getLogger("")

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore", 'bestNSe,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    self.rhoUE = [float(x) for x in row]
                    lineCount += 1
                else:
                    self.bestMode = [int(x) for x in row]

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore" ,'NEx+gammaX2rhoUE,NSe1,T2T132,RRI100.csv') ) as csvFile:#all the values of gammaX and NEx are identical for all NSe so theres no need to get them after the first file read
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    gammaX = [float(x) for x in row[1:]]
                    lineCount += 1
                else:
                    self.NEx.append(int(row[0]))
                    self.NExGammaX2NSe1.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore" ,'NEx+gammaX2rhoUE,NSe2,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    lineCount += 1
                else:
                    self.NExGammaX2NSe2.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore" ,'NEx+gammaX2rhoUE,NSe3,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    lineCount += 1
                else:
                    self.NExGammaX2NSe3.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore" ,'NEx+gammaX2rhoUE,NSe4,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            lineCount = 0
            for row in csvReader:
                if lineCount == 0:
                    lineCount += 1
                else:
                    self.NExGammaX2NSe4.append([float(x) for x in row[1:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore" ,'settingsTable,rhoUE,rhoUEp1,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            for row in csvReader:
                self.settingsChoicesRhoUEp1.append([float(x) for x in row[0:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore" ,'settingsTable,rhoUE,rhoUEp2,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            for row in csvReader:
                self.settingsChoicesRhoUEp2.append([float(x) for x in row[0:]])

        with open(os.path.join(self.ns3_dir, "contrib/ai/examples/nr-ai/ore" ,'settingsTable,rhoUE,rhoUEp4,T2T132,RRI100.csv')) as csvFile:
            csvReader=csv.reader(csvFile,delimiter=',')
            for row in csvReader:
                self.settingsChoicesRhoUEp4.append([float(x) for x in row[0:]])

        for i in range(self.numLanes):
            for j in range(self.numUePerLane):
                self.uePos.append([self.initialPos[0]+self.dV*j, self.initialPos[1] + self.dL*i])

        #zones are numbered 1 to n (n depends on self.dZ). the bounds of the zones start at -2.5,-3 so that when self.dZ = 5 and self.dV = 5, dL = 4 the zones cleanly fit one UE per zone. this also makes it so when self.dZ = 10
        #there are 4 UE per zone and so on upwards. you could make this cleaner in the sense that you could make it so when self.dZ > 5 you move the origin so it always splits the road in two (a zone boundry 
        #exactly on the road divider) but I dont feel that this is a fair approximation of a real world scenario.
        for i in range(self.numUe):
            tempX = 1
            tempY = 1
            while self.uePos[i][0] > self.zoneStart[0] + self.dZ*(tempX):
                tempX += 1
            while self.uePos[i][1] > self.zoneStart[1] + self.dZ*(tempY):
                tempY += 1
            self.zoneId.append(tempX + int((tempY-1)*self.numUePerLane*(self.dV/self.dZ)))

        tempX = 1
        tempY = 1
        for i in range(self.zoneId[len(self.zoneId)-1]):    
            self.zoneCenters.append([self.zoneStart[0] + (tempX-1)*self.dZ + self.dZ/2,self.zoneStart[1] + (tempY-1)*self.dZ + self.dZ/2])
            tempX+=1
            if tempX > self.numUe/2*(self.dV/self.dZ):
                tempX = 1
                tempY+=1

        # logger.info(self.uePos)

    def get_action(self, env):
        logger = logging.getLogger("")
        logger.info(f"ORE get action, selectionModeSelectionFlag {env.selectionModeSelectionFlag}")
        # with open(os.path.join(self.output_dir, 'encodedSelectionInstructions.txt'), 'w') as file:
        #     file.write(str(2) + str(1))
        # return float(str(2) + str(1))
        
        if env.sensingDataFlag == 1:
            self.lastImsi = env.imsi-1-math.floor(env.imsi/256)#this is saved because the functions are called in sequence and NrSlUeMacSchedulerSimple does not have access to imsi. any calls made by NrSlUeMacSchedulerSimple will be related to this imsi though
            self.lastNumExclusions[self.lastImsi] = env.occupiedResources
            self.lastRsrpThreshold[self.lastImsi] = env.rsrpThreshold + 110 - 3
        
            #for all 4 encoded values we have to add a junk value at the begining to ensure that the leading zeros (or values that equal 0) 
            #are not erased by the conversion to double on the c++ side. the last value is an end character for force c++ to finish writing 
            #the string before continuing and must be discarded. If it isn't there though python can try to read the file before it's finished writing, resulting in errors

            if(os.path.isfile(os.path.join(self.output_dir, 'encodedSrcRnti.txt'))): 
                with open(os.path.join(self.output_dir, 'encodedSrcRnti.txt'), 'r') as file:
                    encodedSrcRntiString = file.read()
                encodedSrcRntiString = encodedSrcRntiString[1:(len(encodedSrcRntiString)-1)]
                os.remove(os.path.join(self.output_dir, 'encodedSrcRnti.txt'))
                #logger.info(encodedSrcRntiString)

            if(os.path.isfile(os.path.join(self.output_dir, 'encodedRc.txt'))): 
                with open(os.path.join(self.output_dir, 'encodedRc.txt'), 'r') as file:
                    encodedRcString = file.read()
                encodedRcString = encodedRcString[1:(len(encodedRcString)-1)]
                os.remove(os.path.join(self.output_dir, 'encodedRc.txt'))
                #logger.info(encodedRcString)

            if(os.path.isfile(os.path.join(self.output_dir, 'encodedSrcSlot.txt'))): 
                with open(os.path.join(self.output_dir, 'encodedSrcSlot.txt'), 'r') as file:
                    encodedSrcSlotString = file.read()
                encodedSrcSlotString = encodedSrcSlotString[1:(len(encodedSrcSlotString)-1)]
                os.remove(os.path.join(self.output_dir, 'encodedSrcSlot.txt'))
                #logger.info(encodedSrcSlotString)

            if(os.path.isfile(os.path.join(self.output_dir, 'encodedSrcSc.txt'))): 
                with open(os.path.join(self.output_dir, 'encodedSrcSc.txt'), 'r') as file:
                    encodedSrcScString = file.read()
                encodedSrcScString = encodedSrcScString[1:(len(encodedSrcScString)-1)]
                os.remove(os.path.join(self.output_dir, 'encodedSrcSc.txt'))
                #logger.info(encodedSrcScString)

            self.lastRecievedPackets[self.lastImsi].clear()
            while len(encodedSrcScString) > 0:
                #leading zeros here ar just padding to ensure consistant size, they can be erased by the conversion to int with no issue
                self.lastRecievedPackets[self.lastImsi].append([int(encodedSrcRntiString[0:3]) - 1 - math.floor(int(encodedSrcRntiString[0:3])/256),int(encodedRcString[0:2]),int(encodedSrcSlotString[0:2]),int(encodedSrcScString[0])])

                encodedSrcRntiString = encodedSrcRntiString[3:]
                encodedRcString = encodedRcString[2:]
                encodedSrcSlotString = encodedSrcSlotString[2:]
                encodedSrcScString = encodedSrcScString[1:]

        else:
            if self.lastImsi > -1:#in the first few milliseconds this can be called before any packets are recieved, it doesnt do anything so we just give it dummy values
                logger.info("Finding resources ")
                logger.info(f"Time {env.time}")
                logger.info(self.lastRecievedPackets[self.lastImsi])
                # logger.info(env.time)
                self.selectionModeSelectionCounter+=1
                ########################################################
                #estimating rhoUE
                if (self.lastImsi > (150/self.dV - 1) and self.lastImsi < (750/self.dV - 150/self.dV - 1)) or (self.lastImsi > (900/self.dV - 1) and self.lastImsi < (1500/self.dV - 150/self.dV - 1)):
                    #temp = [abs(x - self.lastRsrpThreshold[self.lastImsi]) for x in gammaX]
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
                #logger.info('*')
                
                #logger.info(len(self.lastRecievedPackets[self.lastImsi]))
                ########################################################
                #estimating dEdge and viable resource counts
                distFromRx = []
                for i in range(len(self.lastRecievedPackets[self.lastImsi])):
                    #logger.info('apple')
                    #logger.info(self.zoneId[self.lastImsi]-1)
                    #logger.info(self.zoneCenters[self.zoneId[self.lastImsi]-1][0])
                    #logger.info(self.lastRecievedPackets[self.lastImsi][i][0])
                    #logger.info(self.zoneCenters[self.zoneId[self.lastRecievedPackets[self.lastImsi][i][0]]-1][0])
                    #logger.info(self.zoneCenters[self.zoneId[self.lastImsi]-1][1])
                    #logger.info(self.zoneCenters[self.zoneId[self.lastRecievedPackets[self.lastImsi][i][0]]-1][1])
                    distFromRx.append(math.sqrt((self.zoneCenters[self.zoneId[self.lastImsi]-1][0] - self.zoneCenters[self.zoneId[self.lastRecievedPackets[self.lastImsi][i][0]]-1][0])**2 + (self.zoneCenters[self.zoneId[self.lastImsi]-1][1] - self.zoneCenters[self.zoneId[self.lastRecievedPackets[self.lastImsi][i][0]]-1][1])**2))
                #if self.lastImsi == 234:
                #    logger.info('****************************************************************************************')
                #    logger.info(distFromRx)
                #logger.info('*')
                if len(distFromRx) > 0:
                    tempEdge = 1
                    temp = 1
                    while temp > self.gamma:
                        edgeUe = [int(x >= (tempEdge*self.dZ)) for x in distFromRx]
                        temp = sum(edgeUe)/len(edgeUe)
                        if temp >= self.gamma:#this is >= rather than just > because in the last itteration if temp and self.gamma are exactly equal we dont want to subtract - in the next step, so we cancel it out here
                            tempEdge += 1
                    #if self.lastImsi == 234:
                    #    logger.info((tempEdge-1)*self.dZ)
                    self.currentDEdgeEstimate[self.lastImsi] = self.beta*((tempEdge-1)*self.dZ) + (1-self.beta)*self.currentDEdgeEstimate[self.lastImsi]
                    #if self.lastImsi == 234:
                    #    logger.info(self.currentDEdgeEstimate[self.lastImsi])
                else:
                    self.currentDEdgeEstimate[self.lastImsi] = self.currentDEdgeEstimate[self.lastImsi]
                
                if self.lastNumExclusions[self.lastImsi] > 0 and self.lastImsi == 75:
                    self.currentRhoUeEstimateCsv.append([env.time] + self.currentRhoUeEstimate)
                    self.currentDEdgeEstimateCsv.append([env.time] + self.currentDEdgeEstimate)
                    self.lastNumExclusionsCsv.append([env.time] + self.lastNumExclusions)
                    self.lastRsrpThresholdCsv.append([env.time] + self.lastRsrpThreshold)
                #    with open('self.currentDEdgeEstimate.csv', 'a', newline='') as csvFile:
                #        writer = csv.writer(csvFile,delimiter=',')
                #        writer.writerow(temp1)
                #    with open('self.currentRhoUeEstimate.csv', 'a', newline='') as csvFile:
                #        writer = csv.writer(csvFile,delimiter=',')
                #        writer.writerow(temp3)
                L = []
                #logger.info('*')
                R = []
                for i in range(len(self.lastRecievedPackets[self.lastImsi])):
                    if distFromRx[i] >= self.currentDEdgeEstimate[self.lastImsi] and (self.zoneCenters[self.zoneId[self.lastImsi]-1][0] - self.zoneCenters[self.zoneId[self.lastRecievedPackets[self.lastImsi][i][0]]-1][0]) < 0:
                        L.append(self.lastRecievedPackets[self.lastImsi][i] + [distFromRx[i]])
                    if distFromRx[i] >= self.currentDEdgeEstimate[self.lastImsi] and (self.zoneCenters[self.zoneId[self.lastImsi]-1][0] - self.zoneCenters[self.zoneId[self.lastRecievedPackets[self.lastImsi][i][0]]-1][0]) > 0:
                        R.append(self.lastRecievedPackets[self.lastImsi][i] + [distFromRx[i]])
                #if self.lastImsi == 234:
                #    logger.info(env.time)
                #logger.info(L)
                #logger.info(R)
                #logger.info('**')
                logger.info(self.currentRhoUeEstimate[self.lastImsi])
                ########################################################
                #getting selection mode
                if self.currentRhoUeEstimate[self.lastImsi] <= .15:
                    if len(L) == 0 or len(R) == 0:
                        selectionMode = 2
                        settingsChoice = len(self.settingsChoicesRhoUEp1)-1
                    else:
                        for i in range(len(self.settingsChoicesRhoUEp1)):
                            if i < len(self.settingsChoicesRhoUEp1)-2:#last row of the settings table is different
                                lCounter = 0
                                rCounter = 0
                                lChoices = []
                                rChoices = []
                                finalChoices = []
                                settingsChoice = -1
                                for j in range(len(L)):
                                    if L[j][1] <=  self.settingsChoicesRhoUEp1[i][1]:
                                        lCounter += 1
                                        lChoices.append(L[j])
                                for j in range(len(R)):
                                    if R[j][1] <=  self.settingsChoicesRhoUEp1[i][1]:
                                        rCounter += 1
                                        rChoices.append(R[j])
                                if (self.settingsChoicesRhoUEp1[i][0] % 2) == 0:
                                    if lCounter >= self.settingsChoicesRhoUEp1[i][0]/2 and rCounter >= self.settingsChoicesRhoUEp1[i][0]/2:
                                        selectionMode = 1#placeholder, needs to be changed to 1 later
                                        settingsChoice = i
                                        sortedLChoices = Sort(lChoices,3)
                                        sortedRChoices = Sort(rChoices,3)
                                        sortedLChoices.reverse()
                                        sortedRChoices.reverse()
                                        self.OReCounter+=1
                                        #logger.info("O-Re")
                                        break
                                else:
                                    if lCounter >= (self.settingsChoicesRhoUEp1[i][0]-1)/2 and rCounter >= (self.settingsChoicesRhoUEp1[i][0]-1)/2:
                                        selectionMode = 1#placeholder, needs to be changed to 1 later
                                        settingsChoice = i
                                        sortedLChoices = Sort(lChoices,3)
                                        sortedRChoices = Sort(rChoices,3)
                                        sortedLChoices.reverse()
                                        sortedRChoices.reverse()
                                        self.OReCounter+=1
                                        #logger.info("O-Re")
                                        break
                            else:   
                                selectionMode = 2#placeholder
                                settingsChoice = i
                elif self.currentRhoUeEstimate[self.lastImsi] > .15 and self.currentRhoUeEstimate[self.lastImsi] <= .3:
                    if len(L) == 0 or len(R) == 0:
                        selectionMode = 2
                        settingsChoice = len(self.settingsChoicesRhoUEp2)-1
                    else:
                        for i in range(len(self.settingsChoicesRhoUEp2)):
                            #logger.info(i)
                            if i < len(self.settingsChoicesRhoUEp2)-2:#last row of the settings table is different
                                lCounter = 0
                                rCounter = 0
                                lChoices = []
                                rChoices = []
                                finalChoices = []
                                settingsChoice = -1
                                for j in range(len(L)):
                                    if L[j][1] <=  self.settingsChoicesRhoUEp2[i][1]:
                                        lCounter += 1
                                        lChoices.append(L[j])
                                for j in range(len(R)):
                                    if R[j][1] <=  self.settingsChoicesRhoUEp2[i][1]:
                                        rCounter += 1
                                        rChoices.append(R[j])
                                if (self.settingsChoicesRhoUEp2[i][0] % 2) == 0:
                                    if lCounter >= self.settingsChoicesRhoUEp2[i][0]/2 and rCounter >= self.settingsChoicesRhoUEp2[i][0]/2:
                                        selectionMode = 1#placeholder, needs to be changed to 1 later
                                        settingsChoice = i
                                        sortedLChoices = Sort(lChoices,3)
                                        sortedRChoices = Sort(rChoices,3)
                                        sortedLChoices.reverse()
                                        sortedRChoices.reverse()
                                        self.OReCounter+=1
                                        #logger.info("O-Re")
                                        break
                                else:
                                    if lCounter >= (self.settingsChoicesRhoUEp2[i][0]-1)/2 and rCounter >= (self.settingsChoicesRhoUEp2[i][0]-1)/2:
                                        selectionMode = 1#placeholder, needs to be changed to 1 later
                                        settingsChoice = i
                                        sortedLChoices = Sort(lChoices,3)
                                        sortedRChoices = Sort(rChoices,3)
                                        sortedLChoices.reverse()
                                        sortedRChoices.reverse()
                                        self.OReCounter+=1
                                        #logger.info("O-Re")
                                        break
                            else:   
                                selectionMode = 2#placeholder
                                settingsChoice = i
                elif self.currentRhoUeEstimate[self.lastImsi] > .3:
                    if len(L) == 0 or len(R) == 0:
                        selectionMode = 2
                        settingsChoice = len(self.settingsChoicesRhoUEp4)-1
                    else:
                        for i in range(len(self.settingsChoicesRhoUEp4)):
                            if i < len(self.settingsChoicesRhoUEp4)-2:#last row of the settings table is different
                                lCounter = 0
                                rCounter = 0
                                lChoices = []
                                rChoices = []
                                finalChoices = []
                                settingsChoice = -1
                                for j in range(len(L)):
                                    if L[j][1] <=  self.settingsChoicesRhoUEp4[i][1]:
                                        lCounter += 1
                                        lChoices.append(L[j])
                                for j in range(len(R)):
                                    if R[j][1] <=  self.settingsChoicesRhoUEp4[i][1]:
                                        rCounter += 1
                                        rChoices.append(R[j])
                                if (self.settingsChoicesRhoUEp4[i][0] % 2) == 0:
                                    if lCounter >= self.settingsChoicesRhoUEp4[i][0]/2 and rCounter >= self.settingsChoicesRhoUEp4[i][0]/2:
                                        selectionMode = 1#placeholder, needs to be changed to 1 later
                                        settingsChoice = i
                                        sortedLChoices = Sort(lChoices,3)
                                        sortedRChoices = Sort(rChoices,3)
                                        sortedLChoices.reverse()
                                        sortedRChoices.reverse()
                                        self.OReCounter+=1
                                        #logger.info("O-Re")
                                        break
                                else:
                                    if lCounter >= (self.settingsChoicesRhoUEp4[i][0]-1)/2 and rCounter >= (self.settingsChoicesRhoUEp4[i][0]-1)/2:
                                        selectionMode = 1#placeholder, needs to be changed to 1 later
                                        settingsChoice = i
                                        sortedLChoices = Sort(lChoices,3)
                                        sortedRChoices = Sort(rChoices,3)
                                        sortedLChoices.reverse()
                                        sortedRChoices.reverse()
                                        self.OReCounter+=1
                                        #logger.info("O-Re")
                                        break
                            else:   
                                selectionMode = 2#placeholder
                                settingsChoice = i
                ########################################################
                #logger.info('*')
                logger.info(selectionMode)
                
                #getting parameters to send depending on selection mode
                if selectionMode == 1:
                    logger.info(sortedLChoices)
                    logger.info(sortedRChoices)
                    if self.currentRhoUeEstimate[self.lastImsi] <= .15:
                        temp = str(selectionMode) + str(int(self.settingsChoicesRhoUEp1[int(settingsChoice)][0]))
                        self.currentNSe[self.lastImsi] = int(self.settingsChoicesRhoUEp1[len(self.settingsChoicesRhoUEp1)-1][2])
                        #logger.info('*')
                        #logger.info(self.settingsChoicesRhoUEp1[int(settingsChoice)][0])
                        #logger.info(type(self.settingsChoicesRhoUEp1[int(settingsChoice)][0]))
                        for i in range(int(self.settingsChoicesRhoUEp1[int(settingsChoice)][0]) - int(self.settingsChoicesRhoUEp1[int(settingsChoice)][0]) % 2):
                            if i % 2 == 0:
                                #logger.info('**')
                                if sortedLChoices[int(i/2)][2] > int(int(round(env.time))%100):
                                    chosenSlot = int(sortedLChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                else:
                                    chosenSlot = int(100 + sortedLChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots

                                chosenSubChannel = int(sortedLChoices[int(i/2)][3])

                                if chosenSlot > 9:
                                    temp = temp + str(chosenSlot) + str(chosenSubChannel)
                                else:#zeros padding to keep size consisant
                                    temp = temp + str(0) + str(chosenSlot) + str(chosenSubChannel)
                            else:
                                #logger.info('***')
                                if sortedRChoices[int(i/2)][2] > int(int(round(env.time))%100):
                                    chosenSlot = int(sortedRChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                else:
                                    chosenSlot = int(100 + sortedRChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                
                                chosenSubChannel = int(sortedRChoices[int((i-1)/2)][3])
                                if chosenSlot > 9:
                                    temp = temp + str(chosenSlot) + str(chosenSubChannel)
                                else:#zeros padding to keep size consisant
                                    temp = temp + str(0) + str(chosenSlot) + str(chosenSubChannel)
                    elif self.currentRhoUeEstimate[self.lastImsi] > .15 and self.currentRhoUeEstimate[self.lastImsi] <= .3:
                        #logger.info('*')
                        temp = str(selectionMode) + str(int(self.settingsChoicesRhoUEp2[int(settingsChoice)][0]))
                        self.currentNSe[self.lastImsi] = int(self.settingsChoicesRhoUEp2[len(self.settingsChoicesRhoUEp2)-1][2])
                        for i in range(int(self.settingsChoicesRhoUEp2[int(settingsChoice)][0]) - int(self.settingsChoicesRhoUEp2[int(settingsChoice)][0]) % 2):
                            if i % 2 == 0:
                                #logger.info('**')
                                if sortedLChoices[int(i/2)][2] > int(int(round(env.time))%100):
                                    chosenSlot = int(sortedLChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                else:
                                    chosenSlot = int(100 + sortedLChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                chosenSubChannel = int(sortedLChoices[int(i/2)][3])
                                if chosenSlot > 9:
                                    temp = temp + str(chosenSlot) + str(chosenSubChannel)
                                else:#zeros padding to keep size consisant
                                    temp = temp + str(0) + str(chosenSlot) + str(chosenSubChannel)
                            else:
                                #logger.info('***')
                                if sortedRChoices[int(i/2)][2] > int(int(round(env.time))%100):
                                    chosenSlot = int(sortedRChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                else:
                                    chosenSlot = int(100 + sortedRChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                chosenSubChannel = int(sortedRChoices[int((i-1)/2)][3])
                                if chosenSlot > 9:
                                    temp = temp + str(chosenSlot) + str(chosenSubChannel)
                                else:#zeros padding to keep size consisant
                                    temp = temp + str(0) + str(chosenSlot) + str(chosenSubChannel)
                    elif self.currentRhoUeEstimate[self.lastImsi] > .3:
                        #logger.info('*')
                        temp = str(selectionMode) + str(int(self.settingsChoicesRhoUEp4[int(settingsChoice)][0]))
                        self.currentNSe[self.lastImsi] = int(self.settingsChoicesRhoUEp4[len(self.settingsChoicesRhoUEp4)-1][2])
                        for i in range(int(self.settingsChoicesRhoUEp4[int(settingsChoice)][0]) - int(self.settingsChoicesRhoUEp4[int(settingsChoice)][0]) % 2):
                            if i % 2 == 0:
                                #logger.info('**')
                                if sortedLChoices[int(i/2)][2] > int(int(round(env.time))%100):
                                    chosenSlot = int(sortedLChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                else:
                                    chosenSlot = int(100 + sortedLChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                chosenSubChannel = int(sortedLChoices[int(i/2)][3])
                                if chosenSlot > 9:
                                    temp = temp + str(chosenSlot) + str(chosenSubChannel)
                                else:#zeros padding to keep size consisant
                                    temp = temp + str(0) + str(chosenSlot) + str(chosenSubChannel)
                            else:
                                #logger.info('***')
                                if sortedRChoices[int(i/2)][2] > int(int(round(env.time))%100):
                                    chosenSlot = int(sortedRChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                else:
                                    chosenSlot = int(100 + sortedRChoices[int(i/2)][2] - (int(int(round(env.time))%100)) - self.t1)#the selection window begins after self.t1 slots
                                chosenSubChannel = int(sortedRChoices[int((i-1)/2)][3])
                                if chosenSlot > 9:
                                    temp = temp + str(chosenSlot) + str(chosenSubChannel)
                                else:#zeros padding to keep size consisant
                                    temp = temp + str(0) + str(chosenSlot) + str(chosenSubChannel)
                    logger.info("apple")
                    encodedSelectionInstructions = float(temp)
                    logger.info(f"Writing selec instruction: {encodedSelectionInstructions}")
                    with open(os.path.join(self.output_dir, 'encodedSelectionInstructions.txt'), 'w') as file:
                        file.write(temp)
                    return encodedSelectionInstructions

                elif selectionMode == 2:
                    if self.currentRhoUeEstimate[self.lastImsi] <= .15:
                        NSe = int(self.settingsChoicesRhoUEp1[int(settingsChoice)][2])
                        self.currentNSe[self.lastImsi] = NSe
                    elif self.currentRhoUeEstimate[self.lastImsi] > .15 and self.currentRhoUeEstimate[self.lastImsi] <= .3:
                        NSe = int(self.settingsChoicesRhoUEp2[int(settingsChoice)][2])
                        self.currentNSe[self.lastImsi] = NSe
                    elif self.currentRhoUeEstimate[self.lastImsi] > .3:
                        NSe = int(self.settingsChoicesRhoUEp4[int(settingsChoice)][2])
                        self.currentNSe[self.lastImsi] = NSe
                    logger.info("banana")
                    encodedSelectionInstructions = float(str(selectionMode) + str(NSe))
                    logger.info(f"Writing selec instruction: {encodedSelectionInstructions}")
                    with open(os.path.join(self.output_dir, 'encodedSelectionInstructions.txt'), 'w') as file:
                        file.write(str(selectionMode) + str(NSe))
                    return encodedSelectionInstructions

            else:#this condition is only triggered in the first few miliseconds, after the initial selection and before any reselection happen. It will be overwritten by the first reselection.
                logger.info("pear")
                #logger.info(float(str(2) + str(1)))
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
                logger.info("ORE run")
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
            logger.info(self.lastNumExclusions)
            logger.info(self.lastRsrpThreshold)
            logger.info(self.currentRhoUeEstimate)
            logger.info(self.currentNSe)
            logger.info(self.selectionModeSelectionCounter)
            logger.info(self.OReCounter)
            with open(os.path.join(self.output_dir,'currentDEdgeEstimate.csv'), 'a', newline='') as csvFile:
                writer = csv.writer(csvFile,delimiter=',')
                writer.writerows(self.currentDEdgeEstimateCsv)
            with open(os.path.join(self.output_dir,'currentRhoUeEstimate.csv'), 'a', newline='') as csvFile:
                writer = csv.writer(csvFile,delimiter=',')
                writer.writerows(self.currentRhoUeEstimateCsv)
            with open(os.path.join(self.output_dir,'lastNumExclusions.csv'), 'a', newline='') as csvFile:
                writer = csv.writer(csvFile,delimiter=',')
                writer.writerows(self.lastNumExclusionsCsv)
            with open(os.path.join(self.output_dir,'lastRsrpThreshold.csv'), 'a', newline='') as csvFile:
                writer = csv.writer(csvFile,delimiter=',')
                writer.writerows(self.lastRsrpThresholdCsv)
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

    # logger.info("Parsed args")
    # logger.info(args)

    _ore = ORE( args.execFilename, args.simulationDirectory, 
                output_dir = args.outputDirectory, 
                ns3_log_filename= args.ns3LogFileName,
                ns3_ai_log_filename = args.aiLogFileName,
                ns3_settings = eval(args.ns3SettingsStr),
            )
    

    _ore.run()



