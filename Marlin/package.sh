#!/usr/bin/env bash

# This script is to package the Marlin package for Arduino
# This script should run under Linux and Mac OS X, as well as Windows with Cygwin.

#############################
# CONFIGURATION
#############################

##Which version name are we appending to the final archive
export BUILD_NAME=16.03.1

#############################
# Actual build script
#############################

if [ -z `which make` ]; then
	MAKE=mingw32-make
else
	MAKE=make
fi


# Change working directory to the directory the script is in
# http://stackoverflow.com/a/246128
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# #For building under MacOS we need gnutar instead of tar
# if [ -z `which gnutar` ]; then
# 	TAR=tar
# else
# 	TAR=gnutar
# fi

#############################
# Build the required firmwares
#############################

if [ -d "C:/arduino-1.0.3" ]; then
	ARDUINO_PATH=C:/arduino-1.0.3
	ARDUINO_VERSION=103
elif [ -d "/Applications/Arduino.app/Contents/Resources/Java" ]; then
	ARDUINO_PATH=/Applications/Arduino.app/Contents/Resources/Java
	ARDUINO_VERSION=105
elif [ -d "C:/Arduino" ]; then
	ARDUINO_PATH=C:/Arduino
	ARDUINO_VERSION=165
else
	ARDUINO_PATH=/usr/share/arduino
	ARDUINO_VERSION=105
fi


#Build the Ultimaker2 firmwares.
# gitClone https://github.com/TinkerGnome/Ultimaker2Marlin.git _Ultimaker2Marlin
# cd _Ultimaker2Marlin/Marlin

# USE_CHANGE_TEMPERATURE

$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2 clean
sleep 2
mkdir _Ultimaker2
$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2 DEFINES="'STRING_CONFIG_H_AUTHOR=\"Tinker_${BUILD_NAME}\"' TEMP_SENSOR_1=0 EXTRUDERS=1 FILAMENT_SENSOR_PIN=30 BABYSTEPPING HEATER_0_MAXTEMP=315 HEATER_1_MAXTEMP=315 HEATER_2_MAXTEMP=315"

cp _Ultimaker2/Marlin.hex resources/firmware/Tinker-MarlinUltimaker2-${BUILD_NAME}.hex

$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2Dual clean
sleep 2
mkdir _Ultimaker2Dual
$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2Dual DEFINES="'STRING_CONFIG_H_AUTHOR=\"Tinker_${BUILD_NAME}\"' TEMP_SENSOR_1=20 EXTRUDERS=2 FILAMENT_SENSOR_PIN=30 BABYSTEPPING HEATER_0_MAXTEMP=315 HEATER_1_MAXTEMP=315 HEATER_2_MAXTEMP=315"
#cd -

cp _Ultimaker2Dual/Marlin.hex resources/firmware/Tinker-MarlinUltimaker2-dual-${BUILD_NAME}.hex

$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2plus clean
sleep 2
mkdir _Ultimaker2plus
$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2plus DEFINES="'STRING_CONFIG_H_AUTHOR=\"Tinker_${BUILD_NAME}+\"' TEMP_SENSOR_1=0 EXTRUDERS=1 FILAMENT_SENSOR_PIN=30 BABYSTEPPING HEATER_0_MAXTEMP=315 HEATER_1_MAXTEMP=315 HEATER_2_MAXTEMP=315 INVERT_E0_DIR=true INVERT_E1_DIR=true INVERT_E2_DIR=true 'EEPROM_VERSION=\"V12\"'"

cp _Ultimaker2plus/Marlin.hex resources/firmware/Tinker-MarlinUltimaker2plus-${BUILD_NAME}.hex
