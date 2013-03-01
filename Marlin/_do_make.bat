@echo off

SET PATH=%PATH%;C:\arduino-1.0.3\hardware\tools\avr\bin
SET PATH=%PATH%;C:\arduino-1.0.3\hardware\tools\avr\utils\bin

SET BASE_PARAMS=HARDWARE_MOTHERBOARD=7 ARDUINO_INSTALL_DIR=C:/arduino-1.0.3 ARDUINO_VERSION=103 
make %BASE_PARAMS% BUILD_DIR=_UltimakerMarlin_250000 DEFINES="'VERSION_PROFILE=\"250000_single\"' BAUDRATE=250000 TEMP_SENSOR_1=0 EXTRUDERS=1"
make %BASE_PARAMS% BUILD_DIR=_UltimakerMarlin_115200 DEFINES="'VERSION_PROFILE=\"115200_single\"' BAUDRATE=115200 TEMP_SENSOR_1=0 EXTRUDERS=1"
make %BASE_PARAMS% BUILD_DIR=_UltimakerMarlin_Dual_250000 DEFINES="'VERSION_PROFILE=\"250000_dual\"' BAUDRATE=250000 TEMP_SENSOR_1=-1 EXTRUDERS=2"
make %BASE_PARAMS% BUILD_DIR=_UltimakerMarlin_Dual_115200 DEFINES="'VERSION_PROFILE=\"115200_dual\"' BAUDRATE=115200 TEMP_SENSOR_1=-1 EXTRUDERS=2"

copy _UltimakerMarlin_250000\Marlin.hex MarlinUltimaker-250000.hex
copy _UltimakerMarlin_115200\Marlin.hex MarlinUltimaker-115200.hex
copy _UltimakerMarlin_Dual_250000\Marlin.hex MarlinUltimaker-250000-dual.hex
copy _UltimakerMarlin_Dual_115200\Marlin.hex MarlinUltimaker-115200-dual.hex

pause