#include "ActivationMode.h"

void ActivationMode(Mode *currentMode, unsigned long uptime,
                    unsigned long *totalUpTime, unsigned long *OBCBootCount)
{
    LoadFRAM();
    // TODO...

    *currentMode = DEPLOYMENT;
}
