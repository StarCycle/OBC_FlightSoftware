/*
 * StateMachine.cpp
 *
 *  Created on: May 19, 2020
 *      Author: tom-h
 */

#include "StateMachine.h"
#include "Communication.h"
#include "ActivationMode.h"
#include "Console.h"
#include "ResetService.h"

extern ResetService reset(GPIO_PORT_P4, GPIO_PIN0);

/**
 *
 *   Define which state it should be and call relative functions.
 *   Override run() in Task.h
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void StateMachine::run() {

    // put TDEM code here

    // How to update the time?

    // If the watch dog isn't kicked in 2.5s, OBC will be reseted
    reset.refreshConfiguration();
    reset.kickExternalWatchDog();




    switch(currentMode) {
        case ACTIVATION:
            ActivationMode();
            break;
        case DEPLOYMENT:
            //run safe mode code
            break;
        case SAFE:
            //run deployment code
            break;
        case ADCS:
            //run ADCS code
            break;
        case NOMINAL:
            //run nominal mode code
            break;
     }
}

/**
 *
 *   Override notified() in Task.h so StateMachine is notified in every loop!
 *
 *   Parameters:
 *
 *   Returns:
 *      true (1)
 */
bool StateMachine::notified() {
    return true;
}

/**
 *
 *   Initial setting. Override setUp() in Task.h
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void StateMachine::setUp() {

    Console::log("Statemachine setup called");

    currentMode = ACTIVATION; // initial mode
    upTime = 0;
    totalUpTime = 0; // TODO
    OBCBootCount = 1; // TODO

    // TODO: load totalUpTime & OBCBootCount from FRAM
}
