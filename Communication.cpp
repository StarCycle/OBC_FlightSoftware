/*
 *  Communication.cpp
 *
 *  Created on: 8 June 2020
 *      Author: Zhuoheng Li
 */

#include "DelfiPQcore.h"
#include "PQ9Frame.h"
#include "PQ9Bus.h"
#include "Console.h"
#include "OBCDataContainer.h"
#include "Communication.h"

#define MAX_PAYLOAD_SIZE    255
#define GET_REPLY           0
#define NO_REPLY            -1


volatile bool cmdReceivedFlag;
PQ9Frame *receivedFrame;
extern PQ9Bus pq9bus(3, GPIO_PORT_P9, GPIO_PIN0); // Defined in main.cpp


/**
 *
 *  Determine active modules according to current mode
 *  Please read Communication.h
 *
 */
void GetActiveModules(Mode currentMode, unsigned char *activeNum, unsigned char activeAddr[])
{
    if ((currentMode == ACTIVATIONMODE) || (currentMode == DEPLOYMENTMODE) || (currentMode == SAFEMODE))
    {
        *activeNum = 3;
        activeAddr[0] = EPS;
        activeAddr[1] = COMMS;
        activeAddr[2] = ADB;
    }
    else if (currentMode == ADCSMODE)
    {
        *activeNum = 4;
        activeAddr[0] = EPS;
        activeAddr[1] = COMMS;
        activeAddr[2] = ADB;
        activeAddr[3] = ADCS;
    }
    else if (currentMode == NOMINALMODE) // TODO: may add payload!
    {
        *activeNum = 4;
        activeAddr[0] = EPS;
        activeAddr[1] = COMMS;
        activeAddr[2] = ADB;
        activeAddr[3] = ADCS;
    }
}


/**
 *
 *  Interrupt service routine when OBC gets a reply
 *  It's registered by pq9bus.setReceiveHandler() in main.cpp
 *
 *  Parameters:
 *      DataFrame &newFrame         Reference of the reply
 *  External variables used:
 *      bool cmdReceivedFlag        It indicates whether OBC gets a reply
 *      PQ9Frame *receivedFrame     Address of the reply
 *
 */
void receivedCommand(DataFrame &newFrame)
{
    cmdReceivedFlag = true;
    receivedFrame = &newFrame;
}


/**
 *
 *  Transmit a pq9frame within the time limit
 *
 *  Parameters:
 *      PQ9Frame sentFrame          The frame which is transmitted
 *      unsigned long timeLimitMS   The time limit
 *  External variables used:
 *      bool cmdReceivedFlag        It indicates whether OBC gets a reply
 *      PQ9Frame *receivedFrame     Address of the reply
 *
 */
void TransmitWithTimeLimit(PQ9Frame sentFrame, unsigned long timeLimitMS)
{
    unsigned long count;

    // Send the frame
    cmdReceivedFlag = false;
    pq9bus.transmit(sentFrame);

    // Wait until timeLimitMS passes or the reply arrives
    count = FCLOCK*timeLimitMS/1000; // TODO: the timer uses FCLOCK?
    MAP_Timer32_initModule(TIMER32_1_BASE, TIMER32_PRESCALER_1, TIMER32_32BIT, TIMER32_PERIODIC_MODE);
    MAP_Timer32_setCount(TIMER32_1_BASE, count);
    MAP_Timer32_startTimer(TIMER32_1_BASE, true);
    while ((MAP_Timer32_getValue(TIMER32_1_BASE) != 0) && (cmdReceivedFlag == false));
}


/**
 *
 *  Send a frame over the bus and get the reply
 *  Please read Communication.h
 *
 */
int RequestReply(Address destination, unsigned char sentSize, unsigned char *sentPayload,
                 unsigned char *receivedSize, unsigned char *receivedPayload,
                 unsigned long timeLimitMS)
{
    PQ9Frame sentFrame;

    if (sentSize > MAX_PAYLOAD_SIZE)
    {
        Console::log("SendFrame(): size of payload is too big: %d bytes", sentSize);
    }

    sentFrame.setSource(OBC);
    sentFrame.setDestination(destination);
    sentFrame.setPayloadSize(sentSize);

    // Copy payload to sentframe
    for (int i = 0; i < sentSize; i++)
    {
        sentFrame.getPayload()[i] = sentPayload[i];
    }

    TransmitWithTimeLimit(sentFrame, timeLimitMS);

    if ((cmdReceivedFlag == true) && (receivedFrame->getSource() == destination))
    {
        receivedPayload = receivedFrame->getPayload();
        *receivedSize = receivedFrame->getPayloadSize();
        return 0;
    }
    else if (cmdReceivedFlag == true)
        return -1;
    else
        return 1;
}

int PingModule(Address destination)
{
    char sentPayload[2];

    sentPayload[0] = 17; // Ping service number
    sentPayload[0] = 1;
}

