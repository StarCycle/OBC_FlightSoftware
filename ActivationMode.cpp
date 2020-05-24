/*
 * ActivationMode.cpp
 *
 *  Created on: May 20, 2020
 *      Author: tom-h
 */

#include "Activationmode.hpp"

using namespace activation;
void SetUpFirstBoot() {
    BootCount = 0;
}
void LoadSD() {
    return;
}
void UpdateBootCount(int bootcount) {
    BootCount = bootcount++;
}

/*
 * Function to check OBC, EPS, Comms
 * and ADB. But I think it will be nice
 * if the things it checks is changable
 *
 * input:
 *          Components to check
 *
 * Output:
 *          for now 1 if correct 0 if not correct
 */
uint8_t HealthCheck() {
    return 42;
}

bool CheckFlag(StateMachine& object) {
    if (TimerDone == true) {
        object.CurrentState = DEPLOYMENT;
        return true;
    }
    else
        return false;
}

void TimerDoneFunc(StateMachine& object) {
    uint64_t Remaining = 1800 - object.upTime;

    if(Remaining<1) {
        object.CurrentState = DEPLOYMENT;
        TimerDone = true;
        }
    else
        return;


}
void Activation() {
    SetUpFirstBoot();
    LoadSD();

    UpdateBootCount(BootCount);

    HealthCheck();

    if (CheckFlag(*StateMachineTask))
        return;
    else
        TimerDoneFunc(*StateMachineTask);
}



