/*
 *  OBCDataContainer.cpp
 *
 *  Created on: Jun 10, 2020
 *      Author: tom-h
 */


#include <OBCDataContainer.h>

int OBCDataContainer::size()
{
    return OBC_DATACONTAINER_SIZE;
}

unsigned char* OBCDataContainer::getArray()
{
    return &data[0];
}

Mode OBCDataContainer::getMode()
{
    return (Mode)data[0];
}

void OBCDataContainer::setMode(Mode currentMode)
{
    data[0] = (unsigned char)currentMode;
}

DeployState OBCDataContainer::getDeployStatus()
{
    int integer;
    ((unsigned char *)&integer)[0] = data[1];
    return integer;
}

void OBCDataContainer::setDeployStatus(DeployState state)
{
    data[1] = state;
}

ADCSState OBCDataContainer::getADCSStatus()
{
    int integer;
    ((unsigned char *)&integer)[0] = data[2];
    return integer;
}

void OBCDataContainer::setADCSStatus(ADCSState state)
{
    data[2] = state;
}

PowerState OBCDataContainer::getADCSPowerStatus()
{
    int integer;
    ((unsigned char *)&integer)[0] = data[3];
    return integer;
}

void OBCDataContainer::setADCSPowerStatus(PowerState state)
{
    data[3] = state;
}

bool OBCDataContainer::getTimerDone()
{
    return ((data[4] & 0x02) != 0);
}

void OBCDataContainer::setTimerDone(bool timerdone)
{
    data[4] &= (~0x02);
    data[4] |= timerdone ? 0x02 : 0x00;
}

bool OBCDataContainer::getDeployment()
{
    return ((data[5] & 0x02) != 0);
}

void OBCDataContainer::setDeployment(bool deploy)
{
    data[5] &= (~0x02);
    data[5] |= deploy ? 0x02 : 0x00;
}

bool OBCDataContainer::getADCSEnable()
{
    return ((data[6] & 0x02) != 0);
}

void OBCDataContainer::setADCSEnable(bool flag)
{
    data[6] &= (~0x02);
    data[6] |= flag ? 0x02 : 0x00;
}
unsigned long OBCDataContainer::getBootCount()
{
    unsigned long ulong;
    ((unsigned char *)&ulong)[3] = data[7];
    ((unsigned char *)&ulong)[2] = data[8];
    ((unsigned char *)&ulong)[1] = data[9];
    ((unsigned char *)&ulong)[0] = data[10];
    return ulong;
}

void OBCDataContainer::setBootCount(unsigned long count)
{
    *((unsigned long *)&(data[7])) = count;
    data[7] = ((unsigned char *)&count)[4];
    data[8] = ((unsigned char *)&count)[3];
    data[9] = ((unsigned char *)&count)[2];
    data[10] = ((unsigned char *)&count)[1];
}

unsigned long OBCDataContainer::getUpTime()
{
    unsigned long ulong;
    ((unsigned char *)&ulong)[3] = data[11];
    ((unsigned char *)&ulong)[2] = data[12];
    ((unsigned char *)&ulong)[1] = data[13];
    ((unsigned char *)&ulong)[0] = data[14];
    return ulong;
}

void OBCDataContainer::setUpTime(unsigned long count)
{
    *((unsigned long *)&(data[11])) = count;
    data[11] = ((unsigned char *)&count)[4];
    data[12] = ((unsigned char *)&count)[3];
    data[13] = ((unsigned char *)&count)[2];
    data[14] = ((unsigned char *)&count)[1];
}

unsigned short OBCDataContainer::getBatteryVoltage()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[15];
    ((unsigned char *)&ushort)[0] = data[16];
    return ushort;
}

void OBCDataContainer::setBatteryVoltage(unsigned short battvolt)
{
    data[15] = ((unsigned char *)&battvolt)[1];
    data[16] = ((unsigned char *)&battvolt)[0];
}

unsigned short OBCDataContainer::getDeployVoltage()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[17];
    ((unsigned char *)&ushort)[0] = data[18];
    return ushort;
}

void OBCDataContainer::setDeployVoltage(unsigned short deployvolt)
{
    data[17] = ((unsigned char *)&deployvolt)[1];
    data[18] = ((unsigned char *)&deployvolt)[0];
}


unsigned short OBCDataContainer::getDeployEnd()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[19];
    ((unsigned char *)&ushort)[0] = data[20];
    return ushort;
}

void OBCDataContainer::setDeployEnd(unsigned short deployend)
{
    data[19] = ((unsigned char *)&deployend)[1];
    data[20] = ((unsigned char *)&deployend)[0];
}

unsigned short OBCDataContainer::getDeployTime()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[21];
    ((unsigned char *)&ushort)[0] = data[22];
    return ushort;
}

void OBCDataContainer::setDeployTime(unsigned short deploytime)
{
    data[21] = ((unsigned char *)&deploytime)[1];
    data[22] = ((unsigned char *)&deploytime)[0];
}

unsigned short OBCDataContainer::getDeployDelay()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[23];
    ((unsigned char *)&ushort)[0] = data[24];
    return ushort;
}

void OBCDataContainer::setDeployDelay(unsigned short deploydelay)
{
    data[23] = ((unsigned char *)&deploydelay)[1];
    data[24] = ((unsigned char *)&deploydelay)[0];
}

unsigned short OBCDataContainer::getSMVoltage()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[25];
    ((unsigned char *)&ushort)[0] = data[26];
    return ushort;
}

void OBCDataContainer::setSMVoltage(unsigned short safevoltage)
{
    data[25] = ((unsigned char *)&safevoltage)[1];
    data[26] = ((unsigned char *)&safevoltage)[0];
}

unsigned short OBCDataContainer::getPowerCycleTime()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[27];
    ((unsigned char *)&ushort)[0] = data[28];
    return ushort;
}

void OBCDataContainer::setPowerCycleTime(unsigned short time)
{
    data[27] = ((unsigned char *)&time)[1];
    data[28] = ((unsigned char *)&time)[0];
}

unsigned short OBCDataContainer::getPowerEnd()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[29];
    ((unsigned char *)&ushort)[0] = data[30];
    return ushort;
}

void OBCDataContainer::setPowerEnd(unsigned short time)
{
    data[29] = ((unsigned char *)&time)[1];
    data[30] = ((unsigned char *)&time)[0];
}

unsigned short OBCDataContainer::getInitEnd()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[31];
    ((unsigned char *)&ushort)[0] = data[32];
    return ushort;
}

void OBCDataContainer::setInitEnd(unsigned short time)
{
    data[31] = ((unsigned char *)&time)[1];
    data[32] = ((unsigned char *)&time)[0];
}

signed short OBCDataContainer::getOmega()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[33];
    ((unsigned char *)&ushort)[0] = data[34];
    return ushort;
}

void OBCDataContainer::setOmega(signed short value)
{
    data[33] = ((unsigned char *)&value)[1];
    data[34] = ((unsigned char *)&value)[0];
}

signed short OBCDataContainer::getMaxOmega()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[35];
    ((unsigned char *)&ushort)[0] = data[36];
    return ushort;
}

void OBCDataContainer::setMaxOmega(signed short value)
{
    data[35] = ((unsigned char *)&value)[1];
    data[36] = ((unsigned char *)&value)[0];
}

unsigned short OBCDataContainer::getDetumbleTime()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[37];
    ((unsigned char *)&ushort)[0] = data[38];
    return ushort;
}

void OBCDataContainer::setDetumbleTime(unsigned short time)
{
    data[37] = ((unsigned char *)&time)[1];
    data[38] = ((unsigned char *)&time)[0];
}

unsigned short OBCDataContainer::getDetumbleEnd()
{
    unsigned short ushort;
    ((unsigned char *)&ushort)[1] = data[39];
    ((unsigned char *)&ushort)[0] = data[40];
    return ushort;
}

void OBCDataContainer::setDetumbleEnd(unsigned short time)
{
    data[39] = ((unsigned char *)&time)[1];
    data[40] = ((unsigned char *)&time)[0];
}
