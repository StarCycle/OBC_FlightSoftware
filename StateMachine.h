/*
 * StateMachine.h
 *
 *  Created on: May 19, 2020
 *      Author: tom-h
 */

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include "Task.h"

class StateMachine : public Task
{
private:
    Mode currentMode;
    uint32_t upTime;
    uint32_t totalUpTime;
    uint32_t OBCBootCount;

protected:
    void run();

public:
    bool notified( void );
    void setUp();
};

#endif /* STATEMACHINE_H_ */
