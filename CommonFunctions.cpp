/*
 * CommonFunctions.h
 *
 *  Created on: 8 June 2020
 *      Author: Zhuoheng Li
 */

#include "CommonFunctions.h"

volatile bool cmdReceivedFlag = false;
DataFrame* receivedFrame;

// TODO: remove when bug in CCS has been solved
void receivedCommand(DataFrame &newFrame)
{
    cmdReceivedFlag = true;
    receivedFrame = &newFrame;
}
