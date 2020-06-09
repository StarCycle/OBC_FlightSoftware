/*
 * ActivationMode.h
 *
 *  Created on: May 20, 2020
 *      Author: tom-h
 */

#include "StateMachine.h"
#include "CommonFunctions.h"

void ActivationMode(Mode *currentMode, unsigned long uptime,
                    unsigned long *totalUpTime, unsigned long *OBCBootCount)
{
    // LoadFRAM();
    // TODO...

    *currentMode = DEPLOYMENT;
}
