#ifndef SDCARD_SIM_H
#define SDCARD_SIM_H

#include "base.h"

class sdcardSimulation : public simBaseComponent
{
private:
    const char* basePath;
public:
    sdcardSimulation(const char* basePath, int errorRate=0);
    virtual ~sdcardSimulation();
    
    void ISP_SPDR_callback(uint8_t oldValue, uint8_t& newValue);
    void read_sd_block(int nr);

    int sd_state;
    uint8_t sd_buffer[1024];
    int sd_buffer_pos;
    int sd_read_block_nr;
    int errorRate;
};

#endif//SDCARD_SIM_H
