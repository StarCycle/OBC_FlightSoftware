/*
 * StateMachine.cpp
 *
 *  Created on: May 19, 2020
 *      Author: tom-h
 */

#include "StateMachine.h"

/**
 *
 *   Construct TaskManager, based on task
 *
 *   Parameters:
 *      const unsigned int count        Period of the task in multiples of 100ms
 *      void (*function)                The function to Execute
 *      void (*init)                    The Initializer of the Function
 *   Returns:
 *      nothing
 *
 */
StateMachine::StateMachine(const unsigned int count, void (*function)( void ), void (&init)( void )) :
        Task(function, init)
{
    this->StateMachineCount = count;
}

/**
 * Run TaskManager
 * Function which reads the state and
 * according to this state runs the corresponding
 * task
 */

void StateMachine::run() {
    switch(CurrentState) {
    case ACTIVATION:
        activation::Activation();
        break;
    case SAFE:
        //run safe mode code
        break;
    case DEPLOYMENT:
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

/*
 * Setup TaskManager task
 * An instance is created of the class
 */

void StateMachine::SetUp() {
#ifndef TESTING
    console::log("Statemachine setup called");
#endif
    (*StateMachineTask).CurrentState = ACTIVATION;

    activation::TimerDone = false;
    //Read TimerDone status from SD

    //read uptime from SD or smth to make sure it is not the uptime since last reboot
    //but since beginning
    //upTimeSD = ReadSD[7];
    //checks if there is a value at the described position, might have to be done
    //differently as there is probably a random value at specified location
//    if(upTimeSD > 0) {
//
//        upTime = upTimeSD;
//    }
//    else
        upTime = 0;
}
