#ifndef SDFILEEXT_H
#define SDFILEEXT_H

#include "Marlin.h"
#ifdef SDSUPPORT

#include "SdFat.h"

/**
 * Defines for long (vfat) filenames
 */
/** Number of VFAT entries used. Every entry has 13 UTF-16 characters */
#define MAX_VFAT_ENTRIES (2)
/** Total size of the buffer used to store the long filenames */
#define LONG_FILENAME_LENGTH (13*MAX_VFAT_ENTRIES+1)

/**
 * \struct directoryVFATEntry
 * \brief VFAT long filename directory entry
 *
 * directoryVFATEntries are found in the same list as normal directoryEntry.
 * But have the attribute field set to DIR_ATT_LONG_NAME.
 *
 * Long filenames are saved in multiple directoryVFATEntries.
 * Each entry containing 13 UTF-16 characters.
 */
struct directoryVFATEntry {
  /**
   * Sequence number. Consists of 2 parts:
   *  bit 6:   indicates first long filename block for the next file
   *  bit 0-4: the position of this long filename block (first block is 1)
   */
  uint8_t sequenceNumber;
  /** First set of UTF-16 characters */
  uint8_t name1[10];//UTF-16
  /** attributes (at the same location as in directoryEntry), always 0x0F */
  uint8_t  attributes;
  /** Reserved for use by Windows NT. Always 0. */
  uint8_t  reservedNT;
  /** Checksum of the short 8.3 filename, can be used to checked if the file system as modified by a not-long-filename aware implementation. */
  uint8_t  checksum;
  /** Second set of UTF-16 characters */
  uint8_t name2[12];//UTF-16
  /** firstClusterLow is always zero for longFilenames */
  uint16_t firstClusterLow;
  /** Third set of UTF-16 characters */
  uint8_t name3[4];//UTF-16
} __attribute__((__packed__));
//------------------------------------------------------------------------------
// Definitions for directory entries
//
/** Type name for directoryVFATEntry */
typedef struct directoryVFATEntry vfat_t;


class SdFileExt : public SdFile {
  public:
    SdFileExt() {}

    int16_t fgets(char* str, int16_t num, char* delim);
    int16_t readDir(dir_t* dir, char* longFilename);
    bool getFilename(char* name);
};

#endif // SDSUPPORT

#endif // SDFILEEXT_H
