/*
 * ActivationMode.cpp
 *
 *  Created on: May 20, 2020
 *      Author: tom-h
 */

#include <ActivationMode.h>


void UpdateBootCount(unsigned long bootcount) {
    bootcount = bootcount++;
}

/*
 * As part of the start up routine, the EPS is commanded to
 * shut down all lines except from line 4. If statement
 * with respect to EPS telemetry can be implemented to send fewer
 * messages when already on/off
 *
 */
bool CommandEPS() {
    bool fault = true;
    char execute = 0x01;
    char request = 0x01;
    char V1 = 0x01;
    char V2 = 0x02;
    char V3 = 0x03;
    char V4 = 0x04;
    char stateon = 0x01; //on
    char stateoff = 0x00;

    //char to store received command
    unsigned char* Reply;
    unsigned char ReplySize;

    //Bitshift according to data structure defined in xml file
    uint32_t On1 = ((static_cast<uint32_t>(stateon) << 24)
        | (static_cast<uint32_t>(request) << 16)
        | (static_cast<uint32_t>(V1) << 8)
        | (static_cast<uint32_t>(execute)));
    uint32_t Off2 = On1 |(static_cast<uint32_t>(V2) <<8)
            | (static_cast<uint32_t>(stateoff));
    uint32_t Off3 = On1 |(static_cast<uint32_t>(V3) <<8)
            | (static_cast<uint32_t>(stateoff));
    uint32_t Off4 = On1 |(static_cast<uint32_t>(V4) <<8)
            | (static_cast<uint32_t>(stateoff));

    //Send to EPS
    int Success1 = RequestReply(EPS, 4, (unsigned char*)&On1, &ReplySize, &Reply, 500);
    int Success2 = RequestReply(EPS, 4, (unsigned char*)&Off2, &ReplySize, &Reply, 500);
    int Success3 = RequestReply(EPS, 4, (unsigned char*)&Off3, &ReplySize, &Reply, 500);
    int Success4 = RequestReply(EPS, 4, (unsigned char*)&Off4, &ReplySize, &Reply, 500);

    if (Success1 ==1 && Success2 ==1 && Success3 ==1 && Success4 ==1) {
        fault = false;
    }
    return fault;

}

bool CheckTimer(unsigned long uptime) {
    unsigned long Remaining = 1801 - uptime;

    if(Remaining<1) {
        return true;
        }
    else
        return false;


}

void Activation(Mode currentMode, unsigned long bootcount, unsigned long uptime) {

    UpdateBootCount(bootcount);

    //command EPS to turn off all power lines except V1, return true if a fault occurs
    bool fault = CommandEPS();

    //todo: do something if fault occurs

    //check if current total uptime is longer than the specified time for deployment
    if (CheckTimer(uptime)) {
        currentMode = DEPLOYMENTMODE;
        return;
    }
    else
        return;
}



