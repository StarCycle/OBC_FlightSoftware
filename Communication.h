/*
 *  Communication.h
 *
 *  Created on: 8 June 2020
 *      Author: Zhuoheng Li
 */

typedef enum Address {OBC = 1, EPS = 2, ADB = 3, COMMS = 4,
    ADCS = 5, PROP = 6, DEBUG = 7, EGSE = 8, HPI = 100} Address;

void PingModules();

