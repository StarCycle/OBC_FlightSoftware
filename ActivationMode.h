/*
 * ActivationMode.h
 *
 *  Created on: May 20, 2020
 *      Author: tom-h
 */

#ifndef ACTIVATIONMODE_H_
#define ACTIVATIONMODE_H_

#include "StateMachine.h"
#include "CommonFunctions.h"

//This function can be called from StateMachine
//and is therefore the only one in this file
void ActivationMode(Mode *currentMode, unsigned long uptime,
                    unsigned long *totalUpTime, unsigned long *OBCBootCount);

#endif /* ACTIVATIONMODE_H_ */
