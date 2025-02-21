# dpdu-passthru
D-PDU API driver for J2534 compatible devices. Currently supports only GM Tech2Win software.

# Currently working
* Any J2534 compatible interface with K-Line support (pin 7)
* Tested with:
  * XHorse Mini-VCI J2534
  * Mongoose Pro JLR
* Tech2Win
  * Saab 9-5 with K-Line diagnostics (pre-2006 year models). The pins 7 and 8 of the DLC need to be shorted in order to access all diagnostic units in the car. Tested with MY2000.
  * Possibly other Saab models which have only K-Line diagnostics

# Todo
* Test more with real cars
* Find out why Saab 9-5 post-2006 fails communication with DICE through the K-Line 
* ISO15765 / CAN support

# Instructions
## Prerequisities
* Install latest Microsoft Visual C++ Redistributable (x86): https://aka.ms/vs/17/release/vc_redist.x86.exe

## Install
1. Dowload latest release: https://github.com/JohnJocke/dpdu-passthru/releases
1. Unzip package and run install.bat as administrator
    
## Build
1. Build the project with Visual Studio 2022, using the Release x86 build configuration
1. Install the driver by running the .bat installer as administrator: https://github.com/JohnJocke/dpdu-passthru/blob/master/installer/install_dev.bat

## Configuration
* Settings file can be found from C:\Users\Public\dpdu_settings.ini
*  Settings:
   * DisableTesterpresent: Set to 1 to disable sending testerpresent messages to the ECU
   * FixTesterpresentDestination: Set to 1 if D-PDU host sends testerpresent messages to wrong destination
   * AutoRestartComm: Set to 1 to automatically restart communication to correct destination
