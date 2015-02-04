#!/usr/bin/env bash

# This script is to package the Marlin package for Arduino
# This script should run under Linux and Mac OS X, as well as Windows with Cygwin.

#############################
# CONFIGURATION
#############################

##Which version name are we appending to the final archive
export BUILD_NAME=15.01

#############################
# Support functions
#############################
function checkTool
{
	if [ -z "`which $1`" ]; then
		echo "The $1 command must be somewhere in your \$PATH."
		echo "Fix your \$PATH or install $2"
		exit 1
	fi
}

function downloadURL
{
	filename=`basename "$1"`
	echo "Checking for $filename"
	if [ ! -f "$filename" ]; then
		echo "Downloading $1"
		curl -L -O "$1"
		if [ $? != 0 ]; then
			echo "Failed to download $1"
			exit 1
		fi
	fi
}

function extract
{
	echo "Extracting $*"
	echo "7z x -y $*" >> log.txt
	7z x -y $* >> log.txt
	if [ $? != 0 ]; then
        echo "Failed to extract $*"
        exit 1
	fi
}

function gitClone
{
	echo "Cloning $1 into $2"
	if [ -d $2 ]; then
		cd $2
		git clean -dfx
		git reset --hard
		git pull
		cd -
	else
		git clone $1 $2
	fi
}

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

checkTool git "git: http://git-scm.com/"
checkTool curl "curl: http://curl.haxx.se/"

checkTool avr-gcc "avr-gcc: http://winavr.sourceforge.net/ "
#Check if we have 7zip, needed to extract and packup a bunch of packages for windows.
checkTool 7z "7zip: http://www.7-zip.org/"
checkTool $MAKE "mingw: http://www.mingw.org/"

#For building under MacOS we need gnutar instead of tar
if [ -z `which gnutar` ]; then
	TAR=tar
else
	TAR=gnutar
fi

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
	ARDUINO_VERSION=106
else
	ARDUINO_PATH=/usr/share/arduino
	ARDUINO_VERSION=105
fi

#Build the Ultimaker2 firmwares.
# gitClone https://github.com/TinkerGnome/Ultimaker2Marlin.git _Ultimaker2Marlin
# cd _Ultimaker2Marlin/Marlin
$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2 DEFINES="'STRING_CONFIG_H_AUTHOR=\"Tinker_${BUILD_NAME}\"' TEMP_SENSOR_1=0 EXTRUDERS=1" clean
sleep 2
mkdir _Ultimaker2
$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2 DEFINES="'STRING_CONFIG_H_AUTHOR=\"Tinker_${BUILD_NAME}\"' TEMP_SENSOR_1=0 EXTRUDERS=1"
$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2Dual DEFINES="'STRING_CONFIG_H_AUTHOR=\"Tinker_${BUILD_NAME}\"' TEMP_SENSOR_1=20 EXTRUDERS=2" clean
sleep 2
mkdir _Ultimaker2Dual
$MAKE -j 3 HARDWARE_MOTHERBOARD=72 ARDUINO_INSTALL_DIR=${ARDUINO_PATH} ARDUINO_VERSION=${ARDUINO_VERSION} BUILD_DIR=_Ultimaker2Dual DEFINES="'STRING_CONFIG_H_AUTHOR=\"Tinker_${BUILD_NAME}\"' TEMP_SENSOR_1=20 EXTRUDERS=2"
# cd -

cp _Ultimaker2/Marlin.hex resources/firmware/TinkerGnome-MarlinUltimaker2-${BUILD_NAME}.hex
cp _Ultimaker2Dual/Marlin.hex resources/firmware/TinkerGnome-MarlinUltimaker2-dual-${BUILD_NAME}.hex

