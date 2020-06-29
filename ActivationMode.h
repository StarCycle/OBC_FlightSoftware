/*
 * ActivationMode.hpp
 *
 *  Created on: May 20, 2020
 *      Author: tom-h
 */
#ifndef ACTIVATIONMODE_HPP_
#define ACTIVATIONMODE_HPP_

#include "StateMachine.h"
#include "Communication.h"
#include "OBCDataContainer.h"
#include "MB85RS.h"

//Function to be called from statemachine
void Activation(Mode currentMode, unsigned long bootcount, unsigned long uptime);



#endif /* ACTIVATIONMODE_HPP_ */

