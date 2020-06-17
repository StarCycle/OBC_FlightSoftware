/*
 *  Communication.h
 *
 *  Created on: 8 June 2020
 *      Author: Zhuoheng Li
 */

// Address number for pq9bus
typedef enum Address {OBC = 1, EPS = 2, ADB = 3, COMMS = 4,
    ADCS = 5, PROP = 6, DEBUG = 7, EGSE = 8, HPI = 100} Address;

/**
 *
 *  Determine active modules according to current mode
 *
 *  Parameters:
 *      Mode currentMode
 *  Returns:
 *      unsigned char *activeNum    Number of active modules except OBC
 *      unsigned char activeAddr[]  Address of these modules
 *  Note: max length of activeAddr[] is 9 (maximum number of active modules)
 *
 */
void GetActiveModules(Mode currentMode, unsigned char *activeNum, unsigned char activeAddr[]);


/**
 *
 *   Send a frame over the bus and get the reply
 *
 *   Parameters:
 *      Address destination             Address of the target board except OBC
 *      unsigned char sentSize          Size of the payload in the sent frame
 *      unsigned char *sentPayload      Payload in the sent frame
 *      unsigned long timeLimitMS       If timeLimitMS expires and OBC doesn't get a reply,
 *                                      return -1
 *   Returns:
 *      SendFrame()                     get reply (0) or no reply (-1) or unknown error (1)
 *      unsigned char *receivedSize     Size of the payload in the received frame
 *      unsigned char *receivedPayload  Payload in the received frame
 *
 */
int RequestReply(Address destination, unsigned char sentSize, unsigned char *sentPayload,
                 unsigned char *receivedSize, unsigned char *receivedPayload,
                 unsigned long timeLimitMS);

// void PingModule();
