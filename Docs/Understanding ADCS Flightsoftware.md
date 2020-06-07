# Basics

**It starts from *main.cpp* in *ADCS_FlightSoftware* folder:**

```c++
// main.cpp in ADCS_FlightSoftware folder

// services running in the system
TestService test;
PingService ping;
ResetService reset( GPIO_PORT_P4, GPIO_PIN0 );
SoftwareUpdateService SWupdate(fram);
HousekeepingService<ADCSTelemetryContainer> hk;
Service* services[] = { &ping, &reset, &hk, &test, &SWupdate };

// ADCS board tasks: cmdHandler (for ISRs) and periodicTasks.
CommandHandler<PQ9Frame> cmdHandler(pq9bus, services, 5); // use the services above
PeriodicTask timerTask(1000, periodicTask);
Task* tasks[] = { &cmdHandler, &timerTask };

// set PeriodicTaskNotifier
PeriodicTask* periodicTasks[] = {&timerTask};
PeriodicTaskNotifier taskNotifier = PeriodicTaskNotifier(periodicTasks, 1); 

// initialize other objects
// ...

void receivedCommand(DataFrame &newFrame) // read the main function
{
    cmdHandler.received(newFrame);
}

void validCmd(void) // read the main function
{
    reset.kickInternalWatchDog();
}

unsigned long uptime = 0; // time since boot
void periodicTask()
{
    // increase the timer, this happens every second. TODO: NOT SAFE!
    uptime++;

    // collect telemetry. Which telemetry should be collect is defined in the next 
    // funtion
    hk.acquireTelemetry(acquireTelemetry);

    // refresh the watch-dog configuration to make sure that, even in case of internal
    // registers corruption, the watch-dog is capable of recovering from an error
    reset.refreshConfiguration();

    // kick hardware watch-dog after every telemetry collection happens
    reset.kickExternalWatchDog();
}

void acquireTelemetry(ADCSTelemetryContainer *tc)
{
    // collect board voltage, temperature, torques...
}

// define other functions
// ...

void main(void)
{
    // initialize hardware and DelfiPQcore; execute Bootloader; get hardware status
    // ...
    
	// pass received frames from the pq9bus object to the cmdHandler object (using 
    // reference and a function pointer)
    pq9bus.setReceiveHandler(&receivedCommand);
    
    // every time a command is correctly processed, call the watch-dog (using a function 
    // pointer)
    cmdHandler.onValidCommand(&validCmd);
    
    Console::log("ADCS booting...SLOT: %d", (int) Bootloader::getCurrentSlot());
    
    TaskManager::start(tasks, 2);
}
```

 Note that:

1. Common functions of different FlightSoftware are put in *DelfiPQcore folder*. Driver functions of different hardware are put in *driver folders* (XXX+number). Functions only for ADCS module are put in the local folder. We use function pointers and reference to transfer functions and data among them:

   DelfiPQcore <-> ADCS module <-> Driver functions

2. All objects are initialized in *main.cpp* in *ADCS_FlightSoftware* folder. That's why we need to link functions from different folders here.

3. Other source codes in *ADCS_FlightSoftware* folder include *ADCSTelemetryContainer* and *TestService*. These functions, as well as the functions written in *DelfiPQcore* and *driver folders*, are finally sent into the *TaskManager*.

4. Both of the CommandHandler and the PeriodciTask are subobjects of Task.

5. CommandHandler is only used in slaves on the bus, like ADCS and EPS. OBC is the master so it doesn't have such thing.

6. There're 2 watchdogs. The internal watchdog is set to 178s, i.e. if no valid command from bus is received in 178s, the watchdog will reset the slave board (for EPS, it will reset the whole satellite, which can be useful if the bus is stuck). On the other hand, the external watchdog is set to 2.5s, which should be kicked by the PeriodicTask, otherwise it will will reset the slave board.



**Let's look at the *TaskManager* in *DelfiPQcore*** (which is cyclic execution with execution flags):

```c++
// TaskManager.h

class TaskManager
{
	public:
    	static void start( Task **tasksArray, int count );
};

// TaskManager.cpp

#include "TaskManager.h"
void TaskManager::start( Task **tasks, int count )
{
    // run the initialization code first
    for (int i = 0; i < count; i++)
    {
        tasks[i]->setUp();
    }

    // run all tasks in a sequence
    while(true)
    {
        // loop through all tasks
        for (int i = 0; i < count; i++)
        {
            tasks[i]->executeTask();
        }
    }
}
```



**What do we have in a Task?**

```c++
// Task.h

class Task
{
 private:
    // Allow TaskManager to access private functions (executeTask)
    // A little complicated. I don't like such design.
    friend class TaskManager;
    void executeTask( void );

 protected:
    volatile bool execute = false; // the execution flag
    
    // Functions are defined outside this object. We save their pointers here.
    void (*userFunction)( void );
    void (*initializerFunction)( void );

    virtual void run(); // Run userFunction. Wrapped by executeTask()

 public:
    // Set *userFunction and *initializerFunction during initialization
    Task( void );
    Task( void (*function)( void ) );
    Task( void (*function)( void ), void (*init)( void ) );
    
    void notify( void ); // Set the flag (i.e. execute) = true
    virtual bool notified( void ); // Read the flag
    
    virtual void setUp( void ); // Run initializerFunction
};

// Task.cpp

#include "Task.h"

/**
 *
 *   Task Constructor: take a user and initializer functions
 *
 *   Parameters:
 *   void (*userFunction)           User function called by the task
 *   void (*initializerFunction)    Initializer function to initialize the task
 *
 *   Returns:
 *
 */
Task::Task( void (*function)( void ), void (*init)( void ) ) :
            userFunction( function ), initializerFunction( init ) {}

// ...

// Weak up the task
void Task::notify( void )
{
    execute = true;

    // make sure the task manager is awaken
    MAP_Interrupt_disableSleepOnIsrExit();
}

// Return if the Task should weak up
bool Task::notified( void )
{
    return execute;
}

// Execute the Task (if execute flag is up) and lower execute flag
void Task::executeTask( void )
{
    if (notified())
    {
        run();
        execute = false;
    }
}

// Task function (passed in constructor or overridden)
void Task::run( void )
{
    if (userFunction)
    {
        userFunction();
    }
}

// Initialize Task using initializer function (passed in constructor or overridden)
void Task::setUp( void )
{
    if (initializerFunction)
    {
        initializerFunction();
    }
}
```



# Command Handler

**Let's look at *CommandHandler* in *DelfiPQcore***:

```c++
// CommandHandler.h

// Different services, external functions and frame types can be used
template <class Frame_Type> 
class CommandHandler: public Task
{
 public:
     // Constructor
     CommandHandler(DataBus &interface, Service **servArray, int count) :
         Task(), bus(interface), services(servArray), servicesCount(count)
     {
         onValidCmd = 0;
         // Load the addresses of rx/tx payloads to rxMsg/txMsg
         rxMsg.setPointer(rxBuffer.getPayload());
         txMsg.setPointer(txBuffer.getPayload());
     };

     // It's linked to the pq9bus object in main.cpp. When a new frame comes, this 		 	  // function will copy the frame to rxBuffer and notify the CommandHandler.   		
     // rxBuffer will be used in run().
     void received( DataFrame &newFrame )
     {
         newFrame.copy(rxBuffer);
         notify();
     };

     // Kick the internal watchdog (defined in main.cpp)
     void onValidCommand(void (*function)( void ))
     {
         onValidCmd = function;
     };
    
 protected:
     DataBus &bus; // A reference of pq9bus, initialized in the constructor
     Service** services; // Service list, initialized in the constructor
     int servicesCount; // Service number, initialized in the constructor
     Frame_Type rxBuffer, txBuffer; // PQ9Frame rxBuffer, txBuffer;
     DataMessage rxMsg, txMsg; // Defined in DataMessage.h
     void (*onValidCmd)( void );

     virtual void run()
     {
         //Prepare Frame to be send back:
         txBuffer.setDestination(rxBuffer.getSource());
         txBuffer.setSource(bus.getAddress());
         rxMsg.setSize(rxBuffer.getPayloadSize());

         if (rxBuffer.getPayloadSize() > 1)
         {
             bool found = false;

             for (int i = 0; i < servicesCount; i++)
             {
                 if (services[i]->process(rxMsg, txMsg)) // Does any of the Services Handle this command?
                 {
                     // stop the loop if a service is found
                     txBuffer.setPayloadSize(txMsg.getSize());
                     bus.transmit(txBuffer);
                     services[i]->postFunc(); // After sending the message
                     found = true;
                     break;
                 }
             }

             if (!found)
             {
                 Console::log("Unknown Service (%d)", (int) rxBuffer.getPayload()[0]);

                 txBuffer.setPayloadSize(2);
                 txBuffer.getPayload()[0] = 0;
                 txBuffer.getPayload()[1] = 0;
                 bus.transmit(txBuffer);
                 return;
             }
             else
             {
                 if (onValidCmd)
                 {
                     onValidCmd(); // Kick the internal watchdog
                 }
                 return;
             }
         }
         else
         {
             // invalid payload size
             Console::log("Invalid Command, size must be > 1");
             txBuffer.setPayloadSize(2);
             txBuffer.getPayload()[0] = 0;
             txBuffer.getPayload()[1] = 0;
             bus.transmit(txBuffer);
             return;
         }
     };
};
```

Note that:

1. CommandHandler **only replies** and will not actively send message to the bus. It is only used in slave boards rather than the OBC.
2. CommandHandler is initialized during compilation. When the pq9bus object gets a new frame from the bus, it will call `received()`, so the new frame is copied to rxBuffer and CommandHandler is notified.
3. Then the TaskManager will call `run()`. It processes rxBuffer, sets the txBuffer and finally `bus.transmit(txBuffer)`. Services are called here.
4. There is a `postFunc()`.  It's useful if the slave board needs to reply quickly before actually doing something. For example, `postFunc()` is used in the reset service.
5. If the slave board receives two frames from the OBC, rxBuffer will only store the latter frame. **(TODO: not sure)**



**How to set the return frame?**

```c++
// DataMessage.h in DelfiPQcore

// Contain the size and address of the payload (a character array)
// We pass a DataMessage rather than a PQ9Frame to services, so they can only set the 
// payload rather than other information (source/destination/...) in the frame.
class DataMessage
{
private:
    unsigned char *payloadBuffer;
    unsigned int payloadSize;

public:
    DataMessage(){
        payloadBuffer = 0;
        payloadSize = 0;
    };
    void setPointer(unsigned char *payloadBufferLocation){
        payloadBuffer = payloadBufferLocation;
    };
    void setSize(unsigned int size){
        payloadSize = size;
    };
    unsigned int getSize(){
        return payloadSize;
    };
    unsigned char* getPayload(){
        return payloadBuffer;
    };
};

// PQ9Frame.h in PQ9Bus

class PQ9Frame : public DataFrame 
{
 protected:
     unsigned char buffer[260]; // The maximum frame size is 260
     CRC16CCITT crc;

     unsigned int frameSize;

 public:
     unsigned char getDestination();
     void setDestination(unsigned char destination);
     unsigned char getSource();
     void setSource(unsigned char source);
     unsigned char getPayloadSize();
     void setPayloadSize(unsigned char size);
     unsigned char *getPayload(); // It's a pointer so we can reset the payload
     void copy(DataFrame &destination);

     virtual void PrepareTransmit();
     unsigned int getFrameSize();
     virtual unsigned char *getFrame();
};

// PQ9Frame.cpp in PQ9Bus

#include "PQ9Frame.h"

unsigned char PQ9Frame::getDestination()
{
    return buffer[0]; // The 1st byte is the destination
}

void PQ9Frame::setDestination(unsigned char destination)
{
    buffer[0] = destination;
}

unsigned char PQ9Frame::getSource()
{
    return buffer[2]; // The 3rd byte is the source
}

void PQ9Frame::setSource(unsigned char source)
{
    buffer[2] = source;
}

unsigned char PQ9Frame::getPayloadSize()
{
    return buffer[1]; // The 2nd byte is the size
}

void PQ9Frame::setPayloadSize(unsigned char size)
{
    buffer[1] = size;
}

unsigned char *PQ9Frame::getPayload()
{
    return &(buffer[3]); // return address of the payload
}

// Copy data in this pq9frame to another frame. It's used in CommandHandler
void PQ9Frame::copy(DataFrame &destination) 
{
    destination.setDestination(getDestination());
    destination.setSource(getSource());
    destination.setPayloadSize(getPayloadSize());
    for (int i = 0; i < getPayloadSize(); i++)
    {
        destination.getPayload()[i] = getPayload()[i];
    }
}

// Add CRC at the end of the frame
void PQ9Frame::PrepareTransmit(){ 
    crc.init();
    crc.newChar(this->getDestination());
    crc.newChar(this->getPayloadSize());
    crc.newChar(this->getSource());
    for (int i = 0; i < this->getPayloadSize(); i++)
    {
        crc.newChar(this->getPayload()[i]);
    }
    unsigned short computedCRC = crc.getCRC();
    buffer[3 + this->getPayloadSize()] = (*((unsigned char*)  &(computedCRC)+1));
    buffer[3 + this->getPayloadSize() + 1] = (*((unsigned char*)  &(computedCRC)+0));
    this->frameSize = this->getPayloadSize() + 5;
}

unsigned int PQ9Frame::getFrameSize(){
    return frameSize;
}

unsigned char* PQ9Frame::getFrame(){
    return &buffer[0];
}

// DataFrame.h in DelfiPQcore (just a framework with virtual functions)

class DataFrame
{
public:
    virtual ~DataFrame(){}
    virtual unsigned char getDestination() = 0;
    virtual void setDestination(unsigned char destination) = 0;
    virtual unsigned char getSource() = 0;
    virtual void setSource(unsigned char source) = 0;
    virtual unsigned char getPayloadSize() = 0;
    virtual void setPayloadSize(unsigned char size) = 0;
    virtual unsigned char *getPayload() = 0;
    virtual void copy(DataFrame &destination) = 0;

    virtual void PrepareTransmit() = 0;
    virtual unsigned int getFrameSize() = 0;
    virtual unsigned char *getFrame() = 0;
};

```



# Services

10 services are defined:

DelfiPQcore:

	- House keeping
	- Ping
	- Reset
	- Software update

PROP_FlightSoftware:

	- Propulsion

XX FlightSoftware:

	- Testing

They are all sub-objects of object **Service**:

```c++
// Service.h in DelfiPQcore

#define SERVICE_RESPONSE_ERROR      0
#define SERVICE_RESPONSE_REQUEST    1
#define SERVICE_RESPONSE_REPLY      2

class Service
{
 protected:
    void (*PostFunction)( void ) = 0;
 public:
    virtual ~Service() {};
    virtual bool process(DataMessage &command, DataMessage &workingBbuffer) = 0;
    void setPostFunc( void (*userFunc)(void)){
        PostFunction = userFunc;
    };
    // Run userFunc only once
    void postFunc(){
        if(PostFunction){
            PostFunction();
            PostFunction = 0;
        }
    };
};
```

Note that process() and postFunc() will be called in CommandHandler.



### Ping service

```c++
// PingService.h in DelfiPQcore

#define PING_SERVICE            17
#define PING_ERROR               0
#define PING_REQUEST             1
#define PING_RESPONSE            2

class PingService: public Service
{
 public:
     virtual bool process( DataMessage &command, DataMessage &workingBbuffer );
};

// PingService.cpp in DelfiPQcore

#include "PingService.h"

bool PingService::process(DataMessage &command, DataMessage &workingBuffer)
{
    if (command.getPayload()[0] == PING_SERVICE) //Check if this frame is directed to this service
    {
        workingBuffer.setSize(command.getSize());// TODO: why???
        workingBuffer.getPayload()[0] = PING_SERVICE;

        if (command.getPayload()[1] == SERVICE_RESPONSE_REQUEST)
        {
            Console::log("PingService: Ping Request");
            // respond to ping
            workingBuffer.getPayload()[1] = SERVICE_RESPONSE_REPLY;
        }
        else
        {
            // unknown request
            workingBuffer.getPayload()[1] = SERVICE_RESPONSE_ERROR;
        }

        return true;
    }
    else
    {
        // this command is related to another service,
        // report the command was not processed
        return false;
    }
}
```

Note that:

1. If the payload of the received frame is SERVICE_RESPONSE_REQUEST (1), the payload of the replying frame will be SERVICE_RESPONSE_REPLY(2).
2. Payload[0] is the type of service. Payload[1] is request/reply/error.
  3. Ping service only checks whether the slave board is "on-line".



### House keeping service

```c++
// HousekeepingService.h in DelfiPQcore

template <class T>
class HousekeepingService: public Service
{
private:
    void stageTelemetry()
    {
        // swap the pointers to the storage / readout entities
        telemetryIndex++;
        telemetryIndex %= 2;
    }
    
    T* getContainerToWrite()
    {
        // return the container not currently used to read the telemetry
        return &(telemetryContainer[telemetryIndex]);
    }

protected:
    int telemetryIndex = 0;
    T telemetryContainer[2];

public:
    bool process( DataMessage &command, DataMessage &workingBuffer )
    {
        if (command.getPayload()[0] == HOUSEKEEPING_SERVICE)
        {
            // prepare response frame
            workingBuffer.getPayload()[0] = HOUSEKEEPING_SERVICE;

            if (command.getPayload()[1] == SERVICE_RESPONSE_REQUEST)
            {
                Console::log("HousekeepingService: Request");

                // respond to housekeeping request
                workingBuffer.getPayload()[1] = SERVICE_RESPONSE_REPLY;
                for (int i = 0; i < getTelemetry()->size(); i++)
                {
                    workingBuffer.getPayload()[i + 2] = getTelemetry()->getArray()[i];
                }
                workingBuffer.setSize(2 + getTelemetry()->size());
            }
            else
            {
                // unknown request
                workingBuffer.getPayload()[1] = SERVICE_RESPONSE_ERROR;
                workingBuffer.setSize(2);
            }

            return true;
        }
        else
        {
            // this command is related to another service,
            // report the command was not processed
            return false;
        }
    }

    T* getTelemetry()
    {
        return &(telemetryContainer[(telemetryIndex + 1) % 2]);
    }

    void acquireTelemetry(void (*callback)( T* ))
    {
        // acquire telemetry via the callback
        callback(getContainerToWrite());

        // telemetry collected, store the values and prepare for next collection
        stageTelemetry();
    }
};
```

Note that:

1. "T" is a C++ template. For ADCS_FlightSoftware, "T" is defined in main.cpp, i.e., `HousekeepingService<ADCSTelemetryContainer> hk;`. Therefore, "T" in this file means `ADCSTelemetryContainer object`, which is defined in ADCS_FlightSoftware. A template enables us to keep a general housekeeping service using telemetry containers.

2. The telemetry container is a "big" container hosting several values, and the values are not written at the same time. This means that, if you query the telemetry while the structure is being populated, you might have some inconsistent values (like uptime and certain voltages). 

   

   To avoid that, **there are 2 containers**: one used for queries and one used to populate the values. These containers are 2 elements of `telemetryContainer` array. We swap them by simply changing the index that points to them (`stageTelemetry()`): the %2 is the remainder of the integer division and it can return only 0 or 1.

   

   This setting may be useless since current TaskHandler is non-preemptive.

3. **How to prepare a response frame:** `telemetryContainer[telemetryIndex+1]` is used. `size()` and `getArray()` are defined in `ADCSTelemetryContainer`. We simply copy the container to the working buffer. **Remember these things are done by CommandHandler.**

4. **How to collect telemetry:** `telemetryContainer[telemetryIndex]` is used. `acquireTelemetry` (defined in main.cpp) is called using a function pointer and the container is set. After that, `stageTelemetry()` will swap the pointers to the storage / readout containers so new telemetry can be transmitted to OBC. **Remember these things are done by PeriodicTask:**

```c++
// main.cpp in ADCS_FlightSoftware

HousekeepingService<ADCSTelemetryContainer> hk;

void periodicTask()
{
    // ...

    // collect telemetry
    hk.acquireTelemetry(acquireTelemetry);

    // ...
}

void acquireTelemetry(ADCSTelemetryContainer *tc)
{
    unsigned short v;
    signed short i, t;

    // set uptime in telemetry
    tc->setUpTime(uptime);

    // measure the power bus
    tc->setBusStatus((!powerBus.getVoltage(v)) & (!powerBus.getCurrent(i)));
    tc->setBusVoltage(v);
    tc->setBusCurrent(i);

    // measure the torquer X
    tc->setTorquerXStatus((!torquerX.getVoltage(v)) & (!torquerX.getCurrent(i)));
    tc->setTorquerXVoltage(v);
    tc->setTorquerXCurrent(i);
    
    // ...
}
```



It's important to look at the ADCS Telemetry Container:

```c++
// ADCSTelemetryContainer.h in ADCS_FlightSoftware

#define ADCS_CONTAINER_SIZE  26

class ADCSTelemetryContainer : public TelemetryContainer
{
protected:
    unsigned char telemetry[ADCS_CONTAINER_SIZE];

public:
    virtual int size();
    virtual unsigned char * getArray();

    unsigned long getUpTime();
    void setUpTime(unsigned long ulong);

    signed short getTemperature();
    void setTemperature(signed short ushort);
    bool getTmpStatus();
    void setTmpStatus(bool bval);

    // ...
};

// ADCSTelemetryContainer.cpp in ADCS_FlightSoftware

#include "ADCSTelemetryContainer.h"

int ADCSTelemetryContainer::size()
{
    return ADCS_CONTAINER_SIZE;
}

unsigned char* ADCSTelemetryContainer::getArray()
{
    return &telemetry[0];
}

unsigned long ADCSTelemetryContainer::getUpTime()
{
    unsigned long ulong;
    ((unsigned char *)&ulong)[3] = telemetry[0];
    ((unsigned char *)&ulong)[2] = telemetry[1];
    ((unsigned char *)&ulong)[1] = telemetry[2];
    ((unsigned char *)&ulong)[0] = telemetry[3];
    return ulong;
}

void ADCSTelemetryContainer::setUpTime(unsigned long ulong)
{
    *((unsigned long *)&(telemetry[0])) = ulong;
    telemetry[0] = ((unsigned char *)&ulong)[3];
    telemetry[1] = ((unsigned char *)&ulong)[2];
    telemetry[2] = ((unsigned char *)&ulong)[1];
    telemetry[3] = ((unsigned char *)&ulong)[0];
}

bool ADCSTelemetryContainer::getBusStatus()
{
    return ((telemetry[7] & 0x02) != 0);
}

void ADCSTelemetryContainer::setBusStatus(bool bval)
{
    telemetry[7] &= (~0x02);
    telemetry[7] |= bval ? 0x02 : 0x00;
}

// ...
```

Note that:

1. The ADCS Telemetry Container is defined in EPS.xml
2. You can visualise the container using XTCEtool:

<img src="C:\Users\Peach\云端同步文件夹\DelfiPQ\代码笔记\xtcetool.PNG" style="zoom:50%;" />

3. We've been discussing about making a tool to translate the XTCE description of the telemetry to a cpp Header/Source file. In this way we don't have to write the cpp manually...

4. Some bits have no meanings (like Sensor0Status, Sensor1Status, Sensor2Status). We need 8 bits to fill telemetry[7] so they are used. XTCE fields always align with bytes in the data array. The next byte field will **not** start "in the middle" of a byte.

5. UpTime is a **long** variable and we see it as 4 **char** variables in the telemetry. That's why you see the type conversion `((unsigned char *)&ulong)[...]` in the code. 

6. XTCE uses MSB first, i.e., the leftmost is the first bit. For example, `TorquerZStatus` is the 4th bit in telemetry[7], and we read it by `(telemetry[7] & 0001 0000B)`, rather than `(telemetry[7] & 0000 1000B)`. 

   There are some wired settings like `((unsigned char *)&ulong)[3] = telemetry[0], which  I can't fully understand...

   

### Reset service

```c++
// ResetService.h in DelfiPQcore

#define RESET_SERVICE           19
#define RESET_ERROR              0
#define RESET_REQUEST            1
#define RESET_RESPONSE           2

#define RESET_SOFT               1
#define RESET_HARD               2
#define RESET_POWERCYCLE         3

class ResetService: public Service
{
 protected:
     const unsigned long WDIPort;
     const unsigned long WDIPin;

 public:
     ResetService( const unsigned long port, const unsigned long pin );
     virtual bool process( DataMessage &command, DataMessage &workingBbuffer );
     void init();

     void refreshConfiguration();
     void kickExternalWatchDog();
     void kickInternalWatchDog();

     void forceHardReset();
     void forceSoftReset();
};

// ResetService.cpp in DelfiPQcore

#include "ResetService.h"

ResetService* resetServiceStub;

void _forceHardReset()
{
    resetServiceStub->forceHardReset();
}

void _forceSoftReset()
{
    resetServiceStub->forceSoftReset();
}

/**
 *
 *   Reset device, triggered by WatchDog interrupt (time-out)
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void resetHandler()
{
    Console::log("ResetService: internal watch-dog reset... ");
    // make sure all characters have been flushed to the console before rebooting
    Console::flush( );

    //Add WDT time=out to reset-cause register
    RSTCTL->HARDRESET_SET |= RESET_HARD_WDTTIME;

    // TODO: replace this with a power cycle to protect also the RS485 driver
    // for now, at least reset, till the power cycle gets implemented in HW
    // MAP_SysCtl_rebootDevice();
    MAP_ResetCtl_initiateHardReset();
}

/**
 *
 *   ResetService Constructor
 *
 *   Parameters:
 *   WDport                     External watch-dog reset port
 *   WDpin                      External watch-dog reset pin
 *
 *   Returns:
 *
 */
ResetService::ResetService(const unsigned long WDport, const unsigned long WDpin) :
    WDIPort(WDport), WDIPin(WDpin) {
    resetServiceStub = this;
}

/**
 *
 *   ResetService Initialize watch-dog service and interrupts.
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void ResetService::init()
{
    // initialize the internal watch-dog
    MAP_WDT_A_clearTimer();                                  // Clear the watch-dog to prevent spurious triggers
    MAP_WDT_A_initIntervalTimer( WDT_A_CLOCKSOURCE_SMCLK,    // set the watch-dog to trigger every 178s
                                 WDT_A_CLOCKITERATIONS_2G ); // (about 3 minutes)

    // select the interrupt handler
    MAP_WDT_A_registerInterrupt(&resetHandler);

    // initialize external watch-dog pins
    MAP_GPIO_setOutputLowOnPin( WDIPort, WDIPin );
    MAP_GPIO_setAsOutputPin( WDIPort, WDIPin );

    // start the timer
    MAP_WDT_A_startTimer();
}

/**
 *
 *   Reset internal watch-dog interrupt and timer and set external watch-dog port/pins as output.
 *
 *   Parameters:

 *   Returns:
 *
 */
void ResetService::refreshConfiguration()
{
    // select the interrupt handler
    MAP_WDT_A_registerInterrupt(&resetHandler);

    // ensure the timer is running: this only forces the
    // timer to run (in case it got disabled for any reason)
    // but it does not reset it, making sure the watch-dog
    // cannot be disabled by mistake
    MAP_WDT_A_startTimer();

    // initialize external watch-dog pins
    MAP_GPIO_setOutputLowOnPin( WDIPort, WDIPin );
    MAP_GPIO_setAsOutputPin( WDIPort, WDIPin );
}

/**
 *
 *   Kick internal watch-dog by resetting timer, should be called every (3min) or reset.
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void ResetService::kickInternalWatchDog()
{
    // reset the internal watch-dog timer
    MAP_WDT_A_clearTimer();
}

/**
 *
 *   Kick external watch-dog by resetting timer.
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void ResetService::kickExternalWatchDog()
{
    // toggle the WDI pin of the external watch-dog
    MAP_GPIO_setOutputHighOnPin( WDIPort, WDIPin );
    MAP_GPIO_setOutputLowOnPin( WDIPort, WDIPin );
}

/**
 *
 *   Process the Service (Called by CommandHandler)
 *
 *   Parameters:
 *   PQ9Frame &command          Frame received over the bus
 *   DataBus &interface       Bus object
 *   PQ9Frame &workingBuffer    Reference to buffer to store the response.
 *
 *   Returns:
 *   bool true      :           Frame is directed to this Service
 *        false     :           Frame is not directed to this Service
 *
 */
bool ResetService::process(DataMessage &command, DataMessage &workingBuffer)
{
    if (command.getPayload()[0] == RESET_SERVICE)
    {
        // prepare response frame
        workingBuffer.setSize(3);
        workingBuffer.getPayload()[0] = RESET_SERVICE;

        if (command.getPayload()[1] == SERVICE_RESPONSE_REQUEST)
        {
            workingBuffer.getPayload()[2] = command.getPayload()[2];
            switch(command.getPayload()[2])
            {
                case RESET_SOFT:
                    workingBuffer.getPayload()[1] = SERVICE_RESPONSE_REPLY;

                    // after a response has been sent, reset the MCU
                    this->setPostFunc(_forceSoftReset);
                    break;

                case RESET_HARD:
                    workingBuffer.getPayload()[1] = SERVICE_RESPONSE_REPLY;

                    // after a response has been sent, force the external watch-dog to reset the MCU
                    this->setPostFunc(_forceHardReset);
                    break;

                default:
                    workingBuffer.getPayload()[1] = SERVICE_RESPONSE_ERROR;
                    break;
            }
        }
        else
        {
            // unknown request
            workingBuffer.getPayload()[1] = RESET_ERROR;
        }

        // command processed
        return true;
    }
    else
    {
        // this command is related to another service,
        // report the command was not processed
        return false;
    }
}

/**
 *
 *   Force the external watch-dog to reset the MCU
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void ResetService::forceHardReset()
{
    Console::log("ResetService: Hard reset");
    // make sure all characters have been flushed to the console before rebooting
    Console::flush( );

    // toggle the WDI pin 3 times, just to be sure
    // the external watch-dog resets...
    MAP_GPIO_setOutputHighOnPin( WDIPort, WDIPin );
    MAP_GPIO_setOutputLowOnPin( WDIPort, WDIPin );
    MAP_GPIO_setOutputHighOnPin( WDIPort, WDIPin );
    MAP_GPIO_setOutputLowOnPin( WDIPort, WDIPin );
    MAP_GPIO_setOutputHighOnPin( WDIPort, WDIPin );
    MAP_GPIO_setOutputLowOnPin( WDIPort, WDIPin );
}

/**
 *
 *   Force the internal watch-dog to reset the MCU
 *
 *   Parameters:
 *
 *   Returns:
 *
 */
void ResetService::forceSoftReset()
{
    Console::log("ResetService: Soft reset");
    // make sure all characters have been flushed to the console before rebooting
    Console::flush( );

    MAP_SysCtl_rebootDevice();
}
```

Note that:

1. `reset.init()` runs in main.cpp once.
2. `reset.refreshConfiguration()` runs in PeriodicTask in every loop.
3. `reset.kickInternalWatchdog()` and `reset.kickExternalWatchdog()` run in CommandHandler and PeriodicTask, respectively.
4. If `reset.process()` gets a frame from CommandHandler, it will set the response frame and the post function. CommandHandler will first reply over the bus and then reset the MCU. A soft reset uses the internal watchdog. A hard reset uses the external watchdog (read document of TSP3813).
5. The time window of the internal watchdog is 178s. For the external watchdog, this value is 2.5s.



# Periodic Tasks

**Let's look at *PeriodicTask* in *DelfiPQcore*:**

```c++
// PeriodicTask.h in DelfiPQcore

class PeriodicTask : public Task
{
public:
    PeriodicTask( const unsigned int count, void (*function)( void ), void (&init)( void ) );
    PeriodicTask( const unsigned int count, void (*function)( void ) );

    int taskCount = 0;
};

// PeriodicTask.cpp in DelfiPQcore

#include "PeriodicTask.h"

PeriodicTask::PeriodicTask(const unsigned int count, void (*function)( void ), void (&init)( void )) :
        Task(function, init)
{
    this->taskCount = count/TASKNOTIFIER_PERIOD_MS;
}

PeriodicTask::PeriodicTask(const unsigned int count, void (*function)( void )) :
        Task(function)
```

Note that:

1. The only difference between a PeriodicTask (derived class) and a Task (basic class) is the variable `taskCount`. 
2. The periodic task notifier runs periodically. The period is `TASKNOTIFIER_PERIOD_MS`.



**Now we look at the periodic task notifier:**

```c++
// PeriodicTaskNotifier.h in DelfiPQcore

#define MAX_PERIODIC_TASKS 50

class PeriodicTaskNotifier
{
private:
    PeriodicTask** taskList;
    int numberOfTasks;
    int taskCounter[MAX_PERIODIC_TASKS] = {0};
    int count;

public:
    PeriodicTaskNotifier(PeriodicTask** taskListIn, int nrOfTasks );
    void NotifyTasks();
    void init();
};

// PeriodicTaskNotifier.cpp in DelfiPQcore

#include "PeriodicTaskNotifier.h"

PeriodicTaskNotifier *notifierStub;

void NotifyTasks_stub(){
    //systick interrupt clears automatically.
    notifierStub->NotifyTasks();
};

PeriodicTaskNotifier::PeriodicTaskNotifier(PeriodicTask** taskListIn, int nrOfTasks ) :
    taskList(taskListIn), numberOfTasks(nrOfTasks){
    notifierStub = this;

    count = FCLOCK/(1000/TASKNOTIFIER_PERIOD_MS);
    static_assert( (FCLOCK/(1000/TASKNOTIFIER_PERIOD_MS)) >> 24 == 0, "PeriodicTaskNotifier Period is only 24 bits!" ); //(assert this is less than 24 bits)
}

void PeriodicTaskNotifier::init(){
    //Halt the Timer and clear the Interrupt flag for compatibility with previous SW versions.
    //( which initializes this timer for periodicTask BEFORE jumping with the bootloader)
    MAP_Timer32_clearInterruptFlag(TIMER32_0_BASE);
    MAP_Timer32_haltTimer(TIMER32_0_BASE);

    MAP_SysTick_enableModule();
    MAP_SysTick_setPeriod(count);  // count is only 24-bits
    MAP_SysTick_registerInterrupt(&NotifyTasks_stub);
    MAP_SysTick_enableInterrupt();
};

void PeriodicTaskNotifier::NotifyTasks(){
    for(int k = 0; k < numberOfTasks; k++){
        this->taskCounter[k] += 1;
        if(this->taskCounter[k] >= this->taskList[k]->taskCount){
            this->taskCounter[k] = 0;
            taskList[k]->notify();
        }
    }
}
```

Note that:

1. `count`  is the crystal oscillation time during a `TASKNOTIFIER_PERIOD_MS`. We send it into `MAP_SysTick_setPeriod()`. 
2. The address of `PeriodicTaskNotifier::NotifyTasks()`, wrapped in `NotifyTasks_stub()`, is sent into `MAP_SysTick_registerInterrupt()`.
3. If the `taskCounter` of the notifier >= the `taskCount` of the PeriodicTask, the task will be notified.



# PQ9Bus



# Bootloader

```c++
// Bootloader.h in DelfiPQcore

class Bootloader{
private:
    MB85RS *fram;
public:
    uint8_t current_slot;
    Bootloader(MB85RS &fram);
    void JumpSlot();
    static unsigned char getCurrentSlot();
};

// Bootloader.cpp in DelfiPQcore 

#include "Bootloader.h"

Bootloader::Bootloader(MB85RS &fram){
    this->fram = &fram;
    this->current_slot = this->getCurrentSlot();
}

unsigned char Bootloader::getCurrentSlot()
{
    // ...
    return slotNumber;
}

void Bootloader::JumpSlot()
{
    uint8_t target_slot = 0;
    current_slot = this->getCurrentSlot();

    if(fram->ping()){
        // Read target_slot from FRAM
        this->fram->read(BOOTLOADER_TARGET_REG, &target_slot, 1);
        // If current slot == 0 and target_slot != 0
        if((current_slot & 0x7F) == 0 && (target_slot & 0x7F) != 0){ 
            //check nr of Reboots to reset targetslot to 0
           uint8_t nrOfReboots = 0;
           fram->read(FRAM_RESET_COUNTER + (target_slot & 0x7F), &nrOfReboots, 1); //get nr 'surprise' reboots of targets
           Console::log("= Target: %d", (int) (target_slot & 0x7F));
           Console::log("= Number of Reboots Target: %d", (int) nrOfReboots);
           if(nrOfReboots > 10 && ((target_slot & 0x7F) != 0)){ //if the surprise reboots >10, reset boot target and reset reboot counter
               Console::log("# Max amount of unintentional reboots!");
               Console::log("# Resetting TargetSlot");
               nrOfReboots = 0;
               fram->write(FRAM_RESET_COUNTER + (target_slot & 0x7F), &nrOfReboots, 1);
               fram->write(BOOTLOADER_TARGET_REG, &current_slot, 1); //reset target to slot0
               fram->read(BOOTLOADER_TARGET_REG, &target_slot, 1);
           }

            //check Succesful boot flag for problems
            uint8_t succesfulBootFlag = 0;
            fram->read(FRAM_BOOT_SUCCES_FLAG, &succesfulBootFlag, 1);
            if(succesfulBootFlag == 0){ //Boot is not succesful, fallback on default slot.
                Console::log("# Last Boot unsuccesful, resetting TargetSlot");
                this->fram->write(BOOTLOADER_TARGET_REG, &current_slot, 1); //reset target to slot0
                this->fram->read(BOOTLOADER_TARGET_REG, &target_slot, 1);
                succesfulBootFlag = 1; //reset bootflag.
                fram->write(FRAM_BOOT_SUCCES_FLAG, &succesfulBootFlag, 1);
            }

            //No problems encountered prep for jump if target is still set
            if((target_slot & 0x7F) != 0){
                Console::log("= Target slot: %d", (int)(target_slot & 0x7F));
                Console::log("= Permanent Jump: %s", ((target_slot & BOOT_PERMANENT_FLAG) > 0) ? "YES" : "NO"); //permanent jump flag is set (not a one time jump)

                if((target_slot & BOOT_PERMANENT_FLAG) == 0) {
                    Console::log("+ Preparing One-time jump");
                    this->fram->write(BOOTLOADER_TARGET_REG, &current_slot, 1); //reset target to slot0
                } else {
                    Console::log("+ Preparing Permanent jump");
                    //this->fram->write(FRAM_TARGET_SLOT, &current_slot, 1); //reset target to slot0
                }

                //lowerBootSuccesFlag before jump
                uint8_t succesfulBootFlag = 0;
                this->fram->write(FRAM_BOOT_SUCCES_FLAG, &succesfulBootFlag, 1);

                MAP_Interrupt_disableMaster();
                MAP_WDT_A_holdTimer();

                uint32_t* resetPtr = 0;
                switch((target_slot & 0x7F)) {
                    case 0:
                        resetPtr = (uint32_t*)(0x00000 + 4);
                        break;
                    case 1:
                        resetPtr = (uint32_t*)(0x20000 + 4);
                        break;
                    case 2:
                        resetPtr = (uint32_t*)(0x30000 + 4);
                        break;
                    default:
                        Console::log("+ BOOTLOADER - Error: target slot not valid!");
                        target_slot = BOOT_PERMANENT_FLAG; //set target to 0 and reboot
                        this->fram->write(BOOTLOADER_TARGET_REG, &target_slot, 1);
                        MAP_SysCtl_rebootDevice();
                        break;
                }
                Console::log("Jumping to: 0x%x", (int) *resetPtr);
                Console::log("=============================================");

                void (*slotPtr)(void) = (void (*)())(*resetPtr);

                slotPtr();  //This is the jump!

                while(1){
                    Console::log("Why are we here?"); //should never end up here
                }
            }else{
                //not jumping anymore
                Console::log("=============================================");
            }

        }else if((current_slot & 0x7F) != 0){
            //In target slot succesfully, hence it is a succesful boot
            uint8_t succesfulBootFlag = 1; //reset bootflag.
            fram->write(FRAM_BOOT_SUCCES_FLAG, &succesfulBootFlag, 1);
            Console::log("=============================================");
        }else{ //In the default slot, but no target is set
            Console::log("=============================================");
        }
    }else{ //fram did not ping
        Console::log("# FRAM Unavailable!");
        Console::log("=============================================");
    }
}
```

Note that:

1. JumpSlot() will be run in main.cpp
2. The Bootloader works according to data in FRAM (both reading and writing).
3. It only jumps when the current slot == 0 and the target slot != 0. If the number of reboots > 10 or boot is not successful, the Bootloader will set target slot = 0 and go back to the default software.
4. Currently we can have a permanent jump or a one-time jump (set the next target slot = 0 before the jump).
5. The Bootloader finally runs the functions in 0x00004, 0x20004 or 0x30004.
6. **I don't fully understand the Bootloader since I am not very familiar with the hardware.**



# Hardware Monitor

```c++
// HWMonitor.h in DelfiPQcore

class HWMonitor
{
 protected:
     MB85RS *fram = 0;
     bool hasFram = false;

     uint32_t resetStatus = 0;
     uint32_t CSStatus = 0;

     uint32_t cal30;
     uint32_t cal85;
     volatile float calDifference;

     uint16_t MCUTemp = 0;
    
 public:
     HWMonitor( MB85RS* fram  );
     HWMonitor();

     void readResetStatus();
     void readCSStatus();

     uint32_t getResetStatus();
     uint32_t getCSStatus();
     uint16_t getMCUTemp();
};

// HWMonitor.cpp in DelfiPQcore

#include "HWMonitor.h"

HWMonitor::HWMonitor(MB85RS* fram_in){
    this->fram = fram_in;
    this->hasFram = true;
}

HWMonitor::HWMonitor(){
    this->hasFram = false;
}

uint32_t HWMonitor::getResetStatus(){
    return resetStatus;
}

uint32_t HWMonitor::getCSStatus(){
    return CSStatus;
}

uint16_t HWMonitor::getMCUTemp(){
    this->MCUTemp = 10*ADCManager::getTempMeasurement();
    return MCUTemp;
}

void HWMonitor::readCSStatus(){
    //Get and clear CLOCK FAULT STATUS
    Console::log("========== HWMonitor: Clock Faults ==========");
    this->CSStatus  = CS->IFG;
    
    Console::log("CS FAULTS: %x", CSStatus);
    
    if( CheckResetSRC(CSStatus, CS_IFG_LFXTIFG)){
        Console::log("- Fault in LFXT");
    }
    if( CheckResetSRC(CSStatus, CS_IFG_HFXTIFG)){
        Console::log("- Fault in HFXT");
    }
    if( CheckResetSRC(CSStatus, CS_IFG_DCOR_SHTIFG)){
        Console::log("- DCO Short Circuit!");
    }
    if( CheckResetSRC(CSStatus, CS_IFG_DCOR_OPNIFG)){
        Console::log("- DCO Open Circuit!");
    }
    if( CheckResetSRC(CSStatus, CS_IFG_FCNTLFIFG)){
        Console::log("- LFXT Start-count expired!");
    }
    if( CheckResetSRC(CSStatus, CS_IFG_FCNTHFIFG)){
        Console::log("- HFXT Start-count expired!");
    }

    CS->CLRIFG |= CS_CLRIFG_CLR_LFXTIFG;
    CS->CLRIFG |= CS_CLRIFG_CLR_HFXTIFG;
    CS->CLRIFG |= CS_CLRIFG_CLR_DCOR_OPNIFG;
    CS->CLRIFG |= CS_CLRIFG_CLR_FCNTLFIFG;
    CS->CLRIFG |= CS_CLRIFG_CLR_FCNTHFIFG;
    CS->CLRIFG |= CS_SETIFG_SET_LFXTIFG;

    Console::log("=============================================");
}

bool CheckResetSRC(uint32_t Code, uint32_t SRC){
    return ((Code & SRC) == SRC);
}

void HWMonitor::readResetStatus(){
    //Get and Clear ResetRegisters
    Console::log("========== HWMonitor: Reboot Cause ==========");
    this->resetStatus  = (RSTCTL->HARDRESET_STAT   & 0x0F) | ((RSTCTL->HARDRESET_STAT & 0xC000) >> 10);
    this->resetStatus |= (RSTCTL->SOFTRESET_STAT   & 0x07) << 6;
    this->resetStatus |= (RSTCTL->PSSRESET_STAT    & 0x0E) << 8;
    this->resetStatus |= (RSTCTL->PCMRESET_STAT    & 0x03) << 12;
    this->resetStatus |= (RSTCTL->PINRESET_STAT    & 0x01) << 14;
    this->resetStatus |= (RSTCTL->REBOOTRESET_STAT & 0x01) << 15;
    this->resetStatus |= (RSTCTL->CSRESET_STAT     & 0x01) << 16;
    MAP_ResetCtl_clearHardResetSource(((uint32_t) 0x0000FFFF));
    MAP_ResetCtl_clearSoftResetSource(((uint32_t) 0x0000FFFF));
    MAP_ResetCtl_clearPSSFlags();
    MAP_ResetCtl_clearPCMFlags();
    RSTCTL->PINRESET_CLR |= (uint32_t) 0x01;
    RSTCTL->REBOOTRESET_CLR |= (uint32_t) 0x01;
    RSTCTL->CSRESET_CLR |= (uint32_t) 0x01;

    Console::log("RESET STATUS: %x", resetStatus);

    if( CheckResetSRC(resetStatus, RESET_HARD_SYSTEMREQ)){
        Console::log("- POR Caused by System Reset Output of Cortex-M4");
    }
    if( CheckResetSRC(resetStatus, RESET_HARD_WDTTIME)){
        Console::log("- POR Caused by HardReset WDT Timer expiration!");
    }
    if( CheckResetSRC(resetStatus, RESET_HARD_WDTPW_SRC)){
        Console::log("- POR Caused by HardReset WDT Wrong Password!");
    }
    if( CheckResetSRC(resetStatus, RESET_HARD_FCTL)){
        Console::log("- POR Caused by FCTL detecting a voltage Anomaly!");
    }
    if( CheckResetSRC(resetStatus, RESET_HARD_CS)){
        Console::log("- POR Extended for Clock Settle!");
    }
    if( CheckResetSRC(resetStatus, RESET_HARD_PCM) ){
        Console::log("- POR Extended for Power Settle!");
    }
    if( CheckResetSRC(resetStatus, RESET_SOFT_CPULOCKUP) ){
        Console::log("- POR Caused by CPU Lock-up!");
    }
    if( CheckResetSRC(resetStatus, RESET_SOFT_WDTTIME) ){
        Console::log("- POR Caused by SoftReset WDT Timer expiration!!");
    }
    if( CheckResetSRC(resetStatus, RESET_SOFT_WDTPW_SRC) ){
        Console::log("- POR Caused by SoftReset WDT Wrong Password!");
    }
    if( CheckResetSRC(resetStatus, RESET_PSS_VCCDET) ){
        Console::log("- POR Caused by VCC Detector trip condition!");
    }
    if( CheckResetSRC(resetStatus, RESET_PSS_SVSH_TRIP) ){
        Console::log("- POR Caused by Supply Supervisor detected Vcc trip condition!");
    }
    if( CheckResetSRC(resetStatus, RESET_PSS_BGREF_BAD) ){
        Console::log("- POR Caused by Bad Band Gap Reference!");
    }
    if( CheckResetSRC(resetStatus, RESET_PCM_LPM35) ){
        Console::log("- POR Caused by PCM due to exit from LPM3.5!");
    }
    if( CheckResetSRC(resetStatus, RESET_PCM_LPM45) ){
        Console::log("- POR Caused by PCM due to exit from LPM4.5!");
    }
    if( CheckResetSRC(resetStatus, RESET_PIN_NMI) ){
        Console::log("- POR Caused by NMI Pin based event!");
    }
    if( CheckResetSRC(resetStatus, RESET_REBOOT) ){
        Console::log("- POR Caused by SysCTL Reboot!");
    }
    if( CheckResetSRC(resetStatus, RESET_CSRESET_DCOSHORT)){
        Console::log("- POR Caused by DCO short circuit fault in external resistor!");
    }

    if(hasFram){
        if(fram->ping()){
            Console::log("+ FRAM present");
            this->fram->write(FRAM_RESET_CAUSE, &((uint8_t*)&resetStatus)[1], 3);
            uint8_t resetCounter = 0;
            Console::log("+ Current Slot: %d", (int) Bootloader::getCurrentSlot());
            fram->read(FRAM_RESET_COUNTER + Bootloader::getCurrentSlot(), &resetCounter, 1);
            if(!CheckResetSRC(resetStatus, RESET_REBOOT)){
                Console::log("+ Unintentional reset!");
                resetCounter++;
                fram->write(FRAM_RESET_COUNTER + Bootloader::getCurrentSlot(), &resetCounter, 1);
            }else{
                Console::log("+ Intentional reset");
                resetCounter = 0;
                fram->write(FRAM_RESET_COUNTER + Bootloader::getCurrentSlot(), &resetCounter, 1);
            }
            Console::log("+ Reset counter at: %d", (int) resetCounter);
        }else{
            Console::log("# FRAM unavailable");
        }
    }else{
        Console::log("# FRAM unavailable");
    }

    Console::log("=============================================");
}
```

Note that:

1. readResetStatus() and readCCStatus() will be run in main.cpp
2. They basically read and them clear registers in the MCU. The results are also shown in the console.
3. Number of **unintended** resets will be recorded in FRAM. It can be used in the Bootloader.
4. A useful function is getMCUTemp(), which calls the ADC Manager.
5. **I don't fully understand the Bootloader since I am not very familiar with the hardware.**



# ADC Manager

```c++
// ADCManager.h in DelfiPQcore

class ADCManager
{
private:
    ADCManager();
    static unsigned int enabledADCMem;
    static unsigned int NrOfActiveADC;
    static const uint32_t MemoryLocations[32];

public:
    static void initADC();
    static void enableTempMeasurement();
    static float getTempMeasurement();
    static void executeADC();
    static int registerADC(const unsigned long ADCPin);
    static uint16_t getMeasurementRaw(int memNumber);
    static uint16_t getMeasurementVolt(int memNumber);
};

// ADCManager.cpp in DelfiPQcore

#include "ADCManager.h"

unsigned int ADCManager::enabledADCMem = 0;
unsigned int ADCManager::NrOfActiveADC = 0;

const uint32_t ADCManager::MemoryLocations[32] = {ADC_MEM0,ADC_MEM1,ADC_MEM2,ADC_MEM3,ADC_MEM4,ADC_MEM5,ADC_MEM6,ADC_MEM7,ADC_MEM8,ADC_MEM9,ADC_MEM10,ADC_MEM11,ADC_MEM12,ADC_MEM13,ADC_MEM14,ADC_MEM15,ADC_MEM16,ADC_MEM17,ADC_MEM18,ADC_MEM19,ADC_MEM20,ADC_MEM21,ADC_MEM22,ADC_MEM23,ADC_MEM24,ADC_MEM25,ADC_MEM26,ADC_MEM27,ADC_MEM28,ADC_MEM29,ADC_MEM30,ADC_MEM31};

void ADCManager::initADC(){
    /* Initializing ADC (MCLK/1/1) with temperature sensor routed */
    // ...

    /* Configuring the sample/hold time for 192 */
    // ...

    /* Enabling sample timer in auto iteration mode and interrupts*/
	// ...
    
    //Enabling the FPU with stacking enabled (for use within ISR)
    // ...

    /* Setting reference voltage to 2.5 and enabling temperature sensor */
    // ...
}

void ADCManager::enableTempMeasurement(){
/* Configuring ADC Memory (ADC_MEM22 A22 (Temperature Sensor) in repeat ) */
    // ...
}

float ADCManager::getTempMeasurement(){
    // ...
}

void ADCManager::executeADC(){
    MAP_ADC14_disableConversion();
    if(NrOfActiveADC == 0){
        return; //no active ADC
    }else if(NrOfActiveADC == 1){
        MAP_ADC14_configureSingleSampleMode(ADC_MEM0, true); //only MEM0 is active
    }else if(NrOfActiveADC < 32){
        //NrOfActive > 0 but not 1
        MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, MemoryLocations[NrOfActiveADC-1], true);
    }else{
        //should never happen
        Console::log("ADCManager: impossible configuration!");
        return;
    }
    /* Triggering the start of the sample */
    MAP_ADC14_enableConversion();
    MAP_ADC14_toggleConversionTrigger();
}

/*
 * registerADC()
 * parameter: (ADC Pin number)
 * returns: ADC Mem Location
 *
 * Description:
 *  Register an Analog Pin to the ADC to be measured continuously.
 *  If a ADC 'slot' is available, the Manager will return a number with the Memory location,
 *  which can be used to obtain the measured result using getMeasurement(int memNumber)
 */
int ADCManager::registerADC(unsigned long ADCPin){
    if((NrOfActiveADC < 32) && (NrOfActiveADC > 0)){ //ADC Slot Available
        int newRegisteredMem = NrOfActiveADC;
        MAP_ADC14_configureConversionMemory(MemoryLocations[newRegisteredMem], ADC_VREFPOS_INTBUF_VREFNEG_VSS, ADCPin, false);

        enabledADCMem |= (1 << newRegisteredMem);
        NrOfActiveADC += 1;
        ADCManager::executeADC();

        return newRegisteredMem;
    } else {
        return -1;
    }
}

/*
 * getMeasurementRaw()
 * parameter: (MEM address number)
 * returns: ADC measurement [in bits]
 *
 * Description:
 *  Get the latest measurement from the ADCManager of the selected Memory
 *
 */
uint16_t ADCManager::getMeasurementRaw(int memNumber){
    return MAP_ADC14_getResult(MemoryLocations[memNumber]);
}

/*
 * getMeasurementRaw()
 * parameter: (MEM address number)
 * returns: ADC measurement [in mV]
 *
 * Description:
 *  Get the latest measurement from the ADCManager of the selected Memory
 *
 */
uint16_t ADCManager::getMeasurementVolt(int memNumber){
    return (MAP_ADC14_getResult(MemoryLocations[memNumber]) * 2500 / 16384);
}
```

Note that:

1. There is a temperature sensor in the AD converter.
2. `executeADC()` kicks off the start of sampling, which works in the repeat mode. In this case, the converter will not stop sampling until the next `MAP_ADC14_toggleConversionTrigger()`
3. `registerADC()` can add a new sampling channel.
4. The results will be saved in `MemoryLocations[memNumber]`, which can be accessed by `getMeasurementRaw()` or `getMeasurementVolt()`.
5. `getTempMeasurement()` returns a float. The FPU is used here.
6. **I don't fully understand the Bootloader since I am not very familiar with the hardware.**

