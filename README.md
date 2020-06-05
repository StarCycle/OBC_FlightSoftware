# OBC_FlightSoftware

Current README file is not well written. 

For more information, please go to the doc folder.

### How to run this software? 

TODO



### Basic idea

We have 2 periodic tasks:

- StateMachine task, which will be run every second
- SDcard task, which only copies data from FRAM to the SD card and will be run every 10 seconds



### Directory structure

| File name           | Description                                                  |
| ------------------- | ------------------------------------------------------------ |
| OBC.h               | Head files, macros and function declarations used in main.cpp |
| main.cpp            | Initialize objects used in OBC flight software and run the TaskManager |
| StateMachine.h      | Declaration of StateMachine() and some common variables      |
| StateMachine.cpp    | Implementation of StateMachine() and these variables         |
| CommonFunctions.h   | Declarations of common functions which can be used in every mode |
| CommonFunctions.cpp | Implementation of common functions                           |
| ActivationMode.h    | Declarations of functions only used in this mode, including ActivationMode() |
| ActivationMode.cpp  | Implementation of these functions                            |
| DeploymentMode.h    | (Not created yet)                                            |
| DeploymentMode.cpp  | (Not created yet)                                            |
| SafeMode.h          | (Not created yet)                                            |
| SafeMode.cpp        | (Not created yet)                                            |
| ADCSMode.h          | (Not created yet)                                            |
| ADCSMode.cpp        | (Not created yet)                                            |
| NominalMode.h       | (Not created yet)                                            |
| NominalMode.cpp     | (Not created yet)                                            |
| SLOT_SELECT.h       | Macros for booting                                           |

