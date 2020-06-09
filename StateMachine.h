/*
 * StateMachine.h
 *
 *  Created on: 5 June 2020
 *      Author: Zhuoheng Li
 */

#ifndef OBCSTATEMACHINE_H_
#define OBCSTATEMACHINE_H_

typedef enum Mode {ACTIVATION, DEPLOYMENT, SAFE, ADCS, NOMINAL} Mode;

void StateMachine();

#endif /* OBCSTATEMACHINE_H_ */
