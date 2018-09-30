* Download the latest version from the <a href="https://github.com/TinkerGnome/Ultimaker2Marlin/releases">Release page</a>
* Description <a href="http://umforum.ultimaker.com/index.php?/topic/10474-tinker-firmware/">en français</a>


### Changelist for this fork:

* new branch geek_mode
* (re-)activated PID-controlling for the printbed temperature
* added new menu option: Maintenance -> Advanced -> Expert settings (activate the "Geek mode" there)
* in "Geek mode" the build-plate wizard is moved into the maintenance menu, the "Advanced" step is skipped
* Redesigned printing screen displaying the Z stage and some tuning parameters
* "instant tuning" - select a parameter and change the target value directly on the printing screen

For more information follow the appropriate discussion on the [Ultimaker forum](http://umforum.ultimaker.com/index.php?/topic/6436-more-information-during-print/page-2#entry84729)



Marlin 3D Printer Firmware
==========================

Quick Information
-----------------
The <a href="https://github.com/">Open Source Marlin project</a> is a firmware for 3D printers based on the RepRap 3D printer hardware and is 
used in 3D printers from many brands. The variant presented here is a fork for use in the Ultimaker printers.
 
The Marlin software was derived from the <a href="https://github.com/kliment/Sprinter">Sprinter</a> and <a href="https://github.com/simen/grbl/tree">grbl</a> projects by Erik van der Zalm.
Many features have been added by community members like: bkubicek, Lampmaker, Bradley Feldman, and others...

Features
---------
*   Interrupt based movement with real linear acceleration
*   High steprate
*   Look ahead (Keep the speed high when possible. High cornering speed)
*   Interrupt based temperature protection
*   SD card support
*   LCD support (ideally 20x4)
*   LCD menu system for autonomous SD card printing, controlled by a click-encoder.
*   EEPROM storage of e.g. max-velocity, max-acceleration, and similar variables
*   Arc support
*   Temperature oversampling
*   Dynamic Temperature setpointing aka "AutoTemp"
*   Heater power reporting. Useful for PID monitoring.
*   PID tuning
*   Automatic operation of extruder/cold-end cooling fans based on nozzle temperature
*   RC Servo Support, specify angle or duration for continuous rotation servos.

The default baudrate is 250000. This baudrate has less jitter and hence errors than the usual 115200 baud, but is less supported by drivers and host-environments.

Implemented G Codes
===================

*  G0  -> G1
*  G1  - Coordinated Movement X Y Z E
*  G2  - CW ARC
*  G3  - CCW ARC
*  G4  - Dwell S[seconds] or P[milliseconds]
*  G10 - retract filament according to settings of M207
*  G11 - retract recover filament according to settings of M208
*  G28 - Home all Axis
*  G90 - Use Absolute Coordinates
*  G91 - Use Relative Coordinates
*  G92 - Set current position to cordinates given

**Manufacturer Codes**
*  M0   - Unconditional stop - Without parameters waits for user to press a button on the LCD, otherwise waits S[seconds] or P[miliseconds].
*  M1   - Same as M0
*  M17  - Enable/Power all stepper motors
*  M18  - Disable all stepper motors; same as M84
*  M20  - List SD card
*  M21  - Init SD card
*  M22  - Release SD card
*  M23  - Select SD file (M23 filename.g)
*  M24  - Start/resume SD print
*  M25  - Pause SD print
*  M26  - Set SD position in bytes (M26 S12345)
*  M27  - Report SD print status
*  M28  - Start SD write (M28 filename.g)
*  M29  - Stop SD write
*  M30  - Delete file from SD (M30 filename.g)
*  M31  - Output time since last M109 or SD card start to serial
*  M42  - Change pin status via gcode Use M42 Px Sy to set pin x to value y, when omitting Px the onboard led will be used.
*  M80  - Turn on Power Supply
*  M81  - Turn off Power Supply
*  M82  - Set E codes absolute (default)
*  M83  - Set E codes relative while in Absolute Coordinates (G90) mode
*  M84  - Disable steppers until next move, or use S[seconds] to specify an inactivity timeout, after which the steppers will be disabled.  S0 to disable the timeout.
*  M85  - Set inactivity shutdown timer with parameter S[seconds]. To disable set zero (default)
*  M92  - Set axis_steps_per_unit - same syntax as G92
*  M104 - Set extruder target temperature.
*  M105 - Read current temperature.
*  M106 - Fan on
*  M107 - Fan off
*  M109 - Wait for extruder to reach target temperature. S[extruder nr]
*  M114 - Output current position to serial port
*  M115 - Capabilities string
*  M117 - Display message.
*  M119 - Output Endstop status to serial port
*  M140 - Set bed target temperature.
*  M190 - Wait for bed to reach target temperature.
*  M201 - Set max acceleration in units/s^2 for print moves (M201 X1000 Y1000)
*  M203 - Set maximum feedrate that your machine can sustain (M203 X200 Y200 Z300 E10000) in mm/sec
*  M204 - Set default acceleration: S[normal moves] T[filament only moves] (M204 S3000 T7000) im mm/sec^2  also sets minimum segment time in ms (B20000) to prevent buffer underruns and M20 minimum feedrate
*  M205 - Advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk, E=maximum E jerk
*  M206 - Set additional homing offset
*  M207 - Set retract length S[positive mm] F[feedrate mm/sec] Z[additional zlift/hop]
*  M208 - Set recover=unretract length S[positive mm surplus to the M207 S*] F[feedrate mm/sec]
*  M209 - S[1=true/0=false] enable automatic retract detect if the slicer did not support G10/11: every normal extrude-only move will be classified as retract depending on the direction.
*  M218 - Set hotend offset, only for printers with multiple extruders (in mm): T[extruder_number] X[offset on X] Y[offset on Y]
*  M220 - Set speed factor override percentage S[factor in percent]
*  M221 - Set extrude factor override percentage for active extruder S[factor in percent]
*  M240 - Trigger a camera to take a photograph (requires compiling with PHOTOGRAPH_PIN)
*  M300 - Play beepsound S[frequency Hz] P[duration ms]
*  M301 - Set PID parameters P, I and D
*  M302 - Allow cold extrudes
*  M303 - PID relay autotune S[temperature] sets the target temperature. (default target temperature = 150C)
*  M304 - Set bed PID parameters P I and D
*  M350 - Set microstepping mode.
*  M351 - Toggle MS1 MS2 pins directly.
*  M400 - Wait for all moves to finish.
*  M401 - Quick stop
*  M500 - stores parameters in EEPROM
*  M501 - reads parameters from EEPROM (if you need reset them after you changed them temporarily).
*  M502 - reverts to the default "factory settings".  You still need to store them in EEPROM afterwards if you want to.
*  M503 - print the current settings (from memory not from eeprom)
*  M540 - Use S[0|1] to enable or disable the stop SD card print on endstop hit (not active in the Ultimaker printers, requires compiling with ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED)
*  M600 - Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]. Experimental feature. Requires compilation with FILAMENTCHANGEENABLE to enable. 
*  M907 - Set digital trimpot motor current using axis codes.
*  M908 - Control digital trimpot directly.
*  M923 - Select file and start printing. (M923 filename.g)
*  M928 - Start SD logging, i.e. all commands output is written to the specified file (M928 filename.g) - ended by M29
*  M999 - Restart after being stopped by error


Configuring and compilation
===========================

Install the Classic Arduino software IDE/toolset v1.0.5
   https://www.arduino.cc/en/Main/OldSoftwareReleases#1.0.x

Copy the Marlin firmware
   https://github.com/Ultimaker/UM2.1-Firmware
   (Use the download button)

The firmware can be built from either the Arduino IDE or from the command line with a make file.

To build with the Arduino IDE
-----------------------------
**One time change (not required when using makefile):**
Ultimaker made changes to the I2C driver. You will have to remove this driver function from the Arduino IDE by removing the entire TWI_vect interrupt routine (located in arduino/libraries/Wire/utility/twi.c, starting at line 364).

* Start the Arduino IDE.
* Select Tools -> Board -> Arduino Mega 2560
* Select the correct serial port in Tools ->Serial Port
* Open the project 'Marlin.pde'
* Click the Verify/Compile button
* Click the Upload button
* If all goes well the firmware is uploading

That's it. Enjoy Silky Smooth Printing.
