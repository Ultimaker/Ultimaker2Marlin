#ifndef LANGUAGE_H
#define LANGUAGE_H

#define PROTOCOL_VERSION "1.1"

#define FIRMWARE_URL ""
#define MACHINE_NAME ""

#define STRINGIFY_(n) #n
#define STRINGIFY(n) STRINGIFY_(n)

// LCD Menu Messages
#define MSG_POWERUP "PowerUp"
#define MSG_EXTERNAL_RESET " External Reset"
#define MSG_BROWNOUT_RESET " Brown out Reset"
#define MSG_WATCHDOG_RESET " Watchdog Reset"
#define MSG_SOFTWARE_RESET " Software Reset"
#define MSG_MARLIN "Marlin "
#define MSG_AUTHOR " | Author: "
#define MSG_CONFIGURATION_VER " Last Updated: "
#define MSG_FREE_MEMORY " Free Memory: "
#define MSG_PLANNER_BUFFER_BYTES "  PlannerBufferBytes: "
#define MSG_OK "ok"

#define MSG_ERR_LINE_NO "Line Number is not Last Line Number+1, Last Line: "
#define MSG_ERR_CHECKSUM_MISMATCH "checksum mismatch, Last Line: "
#define MSG_ERR_NO_CHECKSUM "No Checksum with line number, Last Line: "
#define MSG_ERR_NO_LINENUMBER_WITH_CHECKSUM "No Line Number with checksum, Last Line: "

#define MSG_M115_REPORT "FIRMWARE_NAME:Marlin Ultimaker2; Sprinter/grbl mashup for gen6 FIRMWARE_URL:" FIRMWARE_URL " PROTOCOL_VERSION:" PROTOCOL_VERSION " MACHINE_TYPE:" MACHINE_NAME " EXTRUDER_COUNT:" STRINGIFY(EXTRUDERS) "\n"
#define MSG_ERR_KILLED "Printer halted. kill() called!"
#define MSG_ERR_STOPPED "Printer stopped due to errors. Fix the error and use M999 to restart. (Temperature is reset. Set it after restarting)"
#define MSG_RESEND "Resend: "
#define MSG_UNKNOWN_COMMAND "Unknown command: \""
#define MSG_ACTIVE_EXTRUDER "Active Extruder: "
#define MSG_INVALID_EXTRUDER "Invalid extruder"
#define MSG_M119_REPORT "Reporting endstop status"
#define MSG_X_MIN "x_min: "
#define MSG_X_MAX "x_max: "
#define MSG_Y_MIN "y_min: "
#define MSG_Y_MAX "y_max: "
#define MSG_Z_MIN "z_min: "
#define MSG_Z_MAX "z_max: "
#define MSG_ENDSTOP_HIT "TRIGGERED"
#define MSG_ENDSTOP_OPEN "open"
#define MSG_HOTEND_OFFSET "Hotend offsets:"

#define MSG_STEPPER_TOO_HIGH "Steprate too high: "
#define MSG_ENDSTOPS_HIT "endstops hit: "
#define MSG_ERR_COLD_EXTRUDE_STOP " cold extrusion prevented"
#define MSG_ERR_LONG_EXTRUDE_STOP " too long extrusion prevented"

#endif // ifndef LANGUAGE_H
