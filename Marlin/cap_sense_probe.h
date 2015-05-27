#ifndef CAP_SENSE_PROBE_H
#define CAP_SENSE_PROBE_H

/*
    Probe the bed with the capacitive sensor. Report back the Z position where the bed is hit.
    Does not change the X/Y position. Sets the Z position on the reported Z position.
 */
float probeWithCapacitiveSensor(float x, float y);

#endif//CAP_SENSE_PROBE_H
