/*
 *  OBCDataContainer.h
 *
 *  Created on: Jun 10, 2020
 *      Author: tom-h
 */

#ifndef OBCDATACONTAINER_H_
#define OBCDATACONTAINER_H_

#define OBC_DATACONTAINER_SIZE  40  // TODO

typedef enum Mode {ACTIVATION, DEPLOYMENT, SAFE, ADCS, NOMINAL} Mode;
typedef enum ActivationState  {NOTDONE, DONE} ActivationState;
typedef enum DeployState  {NORMAL, FORCED, DELAYING, DONE} DeployState;
typedef enum ADCSState {IDLE, DETUMBLE, FAILED} ADCSState;

// Can be used in every mode for power line V2, V3 and V4
typedef enum PowerState {INITIALIZING, NORMAL, CYCLING, OFF} PowerState;

class OBCDataContainer
{
protected:
    unsigned char data[OBC_DATACONTAINER_SIZE];

public:
    int size();
    unsigned char * getArray();

    // Variables which are used in every mode

    Mode getMode();
    void setMode(Mode currentMode);

    unsigned long getBootCount();
    void setBootCount(unsigned long count);

    // Uptime since the last boot
    unsigned long getUpTime();
    void setUpTime(unsigned long uplong);

    unsigned long getTotalUpTime();
    void setTotalUpTime(unsigned long uplong);

    unsigned short getBatteryVoltage();
    void setBatteryVoltage(unsigned short battvolt);

    //TODO: something for health check results, make enum probably

    // Variables in the activation mode

    bool getTimerDone();
    void setTimerDone(bool timerdone);

    unsigned long getActivationParameter();
    void getActivationParameter(unsigned long uplong);

    // Variables in the deployment mode

    bool getDeployment();
    void setDeployment(bool deploy);

    unsigned short getDeployVoltage();
    void setDeployVoltage(unsigned short deployvolt);

    DeployState getDeployStatus();
    void setDeployStatus(DeployState state);

    //time assigned for normal deployment after which special measures will be taken
    unsigned short getDeployEnd();
    void setDeployEnd(unsigned short deployend);

    //delay parameter set to alter the deploytime
    unsigned short getDeployTime();
    void setDeployTime(unsigned short delaytime);

    unsigned short getDelayEnd();
    void setDelayEnd(unsigned short delayend);

    unsigned short getDeployDelay();
    void setDeployDelay(unsigned short deploydelay);

    unsigned short getSMVoltage();
    void setSMVoltage(unsigned short safevolt);

    bool getADCSEnable();
    void setADCSEnable(bool flag);

    ADCSState getADCSStatus();
    void setADCSStatus(ADCSState state);

    PowerState getADCSPowerStatus();
    void setADCSPowerStatus(PowerState state);

    //set variable to set time to detumble
    unsigned short getPowerCycleTime();
    void setPowerCycleTime(unsigned short time);

    //time set to indicate power cycling can end, why?
    unsigned short getPowerEnd();
    void setPowerEnd(unsigned short time);

    //time set to indicate initialization time is done
    unsigned short getInitEnd();
    void setInitEnd(unsigned short time);

    signed short getOmega();
    void setOmega(signed short value);

    //treshold set when the rotational speed is acceptable
    signed short getMaxOmega();
    void setMaxOmega(signed short value);

    //Time which the spacecraft will detumble for
    unsigned short getDetumbleTime();
    void setDetumbleTime(unsigned short time);

    //time at which detumbling will end
    unsigned short getDetumbleEnd();
    void setDetumbleEnd(unsigned short time);
};



#endif /* OBCDATACONTAINER_H_ */
