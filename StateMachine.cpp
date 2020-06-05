/*
 * StateMachine.cpp
 *
 *  Created on: 5 June 2020
 *      Author: Zhuoheng Li
 */

#include "StateMachine.h"

enum Mode currentMode = ACTIVATION; // Initial state
unsigned long uptime = 0; // uptime since last boot
unsigned long totalUpTime; // uptime since the first boot
unsigned long OBCBootCount;

void StateMachine()
{
    // Increase the timer, this happens every second
    // There may be time shift of 1s, which is not very important
    uptime++;
    totalUpTime++;

    switch(currentMode)
    {
        case ACTIVATION:
            ActivationMode(&currentMode, uptime, &totalUpTime, &OBCBootCount);
            break;
        case DEPLOYMENT:
            // DeploymentMode(&currentMode, upTime, totalUpTime, OBCBootCount);
            break;
        case SAFE:
            // SafeMode(&currentMode, upTime, totalUpTime, OBCBootCount);
            break;
        case ADCS:
            // ADCSMode(&currentMode, upTime, totalUpTime, OBCBootCount);
            break;
        case NOMINAL:
            // NominalMode(&currentMode, upTime, totalUpTime, OBCBootCount);
            break;
         }
}
