/*
 * StateMachine.h
 *
 *  Created on: 5 June 2020
 *      Author: Zhuoheng Li
 */

#ifndef OBCSTATEMACHINE_H_
#define OBCSTATEMACHINE_H_

// #include "CommonFunctions.h"
#include "ActivationMode.h"
// #include "DeploymentMode.h"
// #include "SafeMode.h"
// #include "ADCSMode.h"
// #include "NominalMode.h"

enum Mode {ACTIVATION, DEPLOYMENT, SAFE, ADCS, NOMINAL};

void StateMachine();

#endif /* OBCSTATEMACHINE_H_ */
