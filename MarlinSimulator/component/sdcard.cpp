#include <dirent.h>
#include <avr/io.h>

#include "sdcard.h"
#include "../../Marlin/SdFatStructs.h"

sdcardSimulation::sdcardSimulation(const char* basePath, int errorRate)
: errorRate(errorRate)
{
    this->basePath = basePath;

    SPDR.setCallback(DELEGATE(registerDelegate, sdcardSimulation, *this, ISP_SPDR_callback));

    sd_state = 0;
    sd_buffer_pos = 0;
}

sdcardSimulation::~sdcardSimulation()
{
}

static const uint16_t crctab[] = {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
  0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
  0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
  0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
  0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
  0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
  0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
  0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
  0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
  0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
  0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
  0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
  0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
  0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
  0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
  0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
  0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
  0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
  0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
  0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
  0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
  0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
  0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
static uint16_t CRC_CCITT(const uint8_t* data, size_t n) {
  uint16_t crc = 0;
  for (size_t i = 0; i < n; i++) {
    crc = crctab[(crc >> 8 ^ data[i]) & 0XFF] ^ (crc << 8);
  }
  return crc;
}

FILE* simFile;
//Total crappy fake FAT32 simulation, works for 1 file, sort of.
void sdcardSimulation::read_sd_block(int nr)
{
    memset(sd_buffer, 0, 512);
    switch(nr)
    {
    case 0x000:{//MBR
        mbr_t* mbr = (mbr_t*)sd_buffer;
        mbr->part[0].boot = 0;
        mbr->part[0].totalSectors = 0x200000;
        mbr->part[0].firstSector = 1;
        }break;
    case 0x001:{//fat32_boot_t
        fat32_boot_t* boot = (fat32_boot_t*)sd_buffer;
        boot->bytesPerSector = 512;
        boot->fatCount = 1;
        boot->reservedSectorCount = 1;
        boot->sectorsPerCluster = 1;
        boot->sectorsPerFat32 = 0x400;
        boot->rootDirEntryCount = 0;
        boot->fat32RootCluster = 1;
        }break;
    case 0x002:{//FAT32 for root dir entries
        uint32_t* fat32 = (uint32_t*)sd_buffer;
        for(uint8_t n=0;n<128;n++)
            fat32[n] = 0X0FFFFFF8;
        for(uint8_t n=0;n<128;n++)
            fat32[n] = n + 1;
        fat32[127] = 0X0FFFFFF8;
        }break;
    default:
        if (nr >= 0x401 && nr < 0x500) //root directory: dir_t
        {
            dir_t* dir = (dir_t*)sd_buffer;

            DIR* dh = opendir(basePath);
            struct dirent *entry;
            int idx = 0;
            int reqIdx = nr - 0x401;
            while((entry=readdir(dh))!=NULL)
            {
                if (entry->d_name[0] == '.')
                    continue;
                if (idx == reqIdx)
                    break;
                idx++;
            }
            if (entry == NULL)
                return;

            const char* namePtr = entry->d_name;

            int fatNr = 0;
            while(*namePtr)
            {
                vfat_t *vfat = (vfat_t*)dir;
                vfat->firstClusterLow = 0;
                vfat->sequenceNumber = 0x41 + fatNr;
                for(unsigned int n=0;n<5&&*namePtr;n++)
                    vfat->name1[n*2] = *namePtr++;
                for(unsigned int n=0;n<6&&*namePtr;n++)
                    vfat->name2[n*2] = *namePtr++;
                for(unsigned int n=0;n<2&&*namePtr;n++)
                    vfat->name3[n*2] = *namePtr++;
                vfat->attributes = DIR_ATT_LONG_NAME;
                dir++;
                fatNr++;
            }

            dir->attributes = 0;
            memcpy(dir->name, "FAKEFILEXCO", 8+3);
            if (strrchr(entry->d_name, '.'))
                dir->name[8] = toupper(strrchr(entry->d_name, '.')[1]);
            dir->firstClusterHigh = 0;
            dir->firstClusterLow = 256;
            if (simFile == NULL)
                simFile = fopen("c:/models/Box_20x20x10.gcode", "rb");
            fseek(simFile, 0, SEEK_END);
            dir->fileSize = ftell(simFile);
            fseek(simFile, 0, SEEK_SET);

            dir++;
            fatNr++;

            closedir(dh);

            while(fatNr < 16)
            {
                dir->attributes = DIR_ATT_HIDDEN;
                dir->name[0] = DIR_NAME_DELETED;

                dir++;
                fatNr++;
            }
        }
        else if (nr >= 4 && nr < 0x400)
        {
            //FAT32 tables for file, just link to the next block
            uint32_t* fat32 = (uint32_t*)sd_buffer;
            for(uint8_t n=0;n<128;n++)
                fat32[n] = (nr-2) * 128 + n + 1;
        }
        else if (nr >= 0x500 && nr < 0x20000)
        {
            //Actual data blocks
            fseek(simFile, (nr - 0x501) * 512, SEEK_SET);
            fread(sd_buffer, 512, 1, simFile);
        }else{
            memset(sd_buffer, 0xFF, 512);
            printf("Read SD?: %x\n", nr);
            //exit(1);
        }
        break;
    }

    uint16_t crc = CRC_CCITT(sd_buffer, 512);
    sd_buffer[512] = crc >> 8;
    sd_buffer[513] = crc;
}

void sdcardSimulation::ISP_SPDR_callback(uint8_t oldValue, uint8_t& newValue)
{
    if (errorRate && (rand() % errorRate) == 0)
        newValue = rand();
    if ((PING & _BV(2)))
    {
        //No card inserted, return 0xFF
        newValue = 0xFF;
        SPSR |= _BV(SPIF);//Mark transfer finished
        return;
    }
    switch(sd_state)
    {
    case 0://Read CMD
    case 2://Read ACMD
        sd_buffer[sd_buffer_pos++] = newValue;
        if (sd_buffer_pos == 1 && (sd_buffer[0] & 0xC0) != 0x40)
            sd_buffer_pos = 0;
        if (sd_buffer_pos == 6) // 1 cmd, 4 param, 1 crc
            sd_state += 1;
        break;
    case 1://Return status of CMD
        sd_state = 0;
        sd_buffer_pos = 0;

        switch(sd_buffer[0] & 0x3F)
        {
        case 0x00://CMD0 - GO_IDLE_STATE
            newValue = 0x01;         //Report R1_IDLE_STATE
            break;
        case 0x08://CMD8 - SEND_IF_COND
            newValue = 0x04;         //Report R1_ILLEGAL_COMMAND
            break;
        case 0x0C://CMD12 - STOP_TRANSMISSION
            newValue = 0x01;         //Report R1_IDLE_STATE
            break;
        case 0x11://CMD17 - READ_SINGLE_BLOCK
            newValue = 0x00;//R1_READY_STATE
            sd_read_block_nr = (sd_buffer[1] << 24) | (sd_buffer[2] << 16) | (sd_buffer[3] << 8) | (sd_buffer[4] << 0);
            read_sd_block(sd_read_block_nr >> 9);
            sd_state = 10;
            sd_buffer_pos = 0;
            break;
        case 0x37://CMD55 - APP_CMD
            sd_state = 2;
            break;
        default:
            printf("SD CMD: %02x %02x %02x %02x %02x %02x\n", sd_buffer[0] & 0x3F, sd_buffer[1], sd_buffer[2], sd_buffer[3], sd_buffer[4], sd_buffer[5]);
            break;
        }
        break;
    case 3://Return status of ACMD
        sd_state = 0;
        sd_buffer_pos = 0;

        switch(sd_buffer[0] & 0x3F)
        {
        case 0x29://ACMD41 - SD_SEND_OP_COMD
            newValue = 0x00;//R1_READY_STATE
            break;
        default:
            printf("SD ACMD: %02x %02x %02x %02x %02x %02x\n", sd_buffer[0] & 0x3F, sd_buffer[1], sd_buffer[2], sd_buffer[3], sd_buffer[4], sd_buffer[5]);
            break;
        }
        break;
    case 10://READ BLOCK
        if (sd_buffer_pos == 0)
            newValue = 0xFE;//DATA_START_BLOCK
        else
            newValue = sd_buffer[sd_buffer_pos-1];

        sd_buffer_pos++;
        if (sd_buffer_pos == 512 + 1 + 2)
        {
            sd_state = 0;
            sd_buffer_pos = 0;
        }
        break;
    }
    //Introduce random errors in SD communication
    if (errorRate && (rand() % errorRate) == 0)
        newValue = rand();

    //Mark transfer finished
    SPSR |= _BV(SPIF);
}
