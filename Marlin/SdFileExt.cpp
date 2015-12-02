#include "Marlin.h"
#ifdef SDSUPPORT

#include "SdFileExt.h"


//------------------------------------------------------------------------------
/**
 * Get a string from a file.
 *
 * fgets() reads bytes from a file into the array pointed to by \a str, until
 * \a num - 1 bytes are read, or a delimiter is read and transferred to \a str,
 * or end-of-file is encountered. The string is then terminated
 * with a null byte.
 *
 * fgets() deletes CR, '\\r', from the string.  This insures only a '\\n'
 * terminates the string for Windows text files which use CRLF for newline.
 *
 * \param[out] str Pointer to the array where the string is stored.
 * \param[in] num Maximum number of characters to be read
 * (including the final null byte). Usually the length
 * of the array \a str is used.
 * \param[in] delim Optional set of delimiters. The default is "\n".
 *
 * \return For success fgets() returns the length of the string in \a str.
 * If no data is read, fgets() returns zero for EOF or -1 if an error occurred.
 **/
int16_t SdFileExt::fgets(char* str, int16_t num, char* delim)
{
  char ch;
  int16_t n = 0;
  int16_t r = -1;
  while ((n + 1) < num && (r = read(&ch, 1)) == 1) {
    // delete CR
    if (ch == '\r') continue;
    str[n++] = ch;
    if (!delim) {
      if (ch == '\n') break;
    } else {
      if (strchr(delim, ch)) break;
    }
  }
  if (r < 0) {
    // read error
    return -1;
  }
  str[n] = '\0';
  return n;
}

//------------------------------------------------------------------------------
/** Get a file's name
 *
 * \param[out] name An array of 13 characters for the file's name.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
bool SdFileExt::getFilename(char* name) {
  if (!isOpen()) return false;

  if (isRoot()) {
    name[0] = '/';
    name[1] = '\0';
    return true;
  }
  // cache entry
  dir_t* p = cacheDirEntry(SdVolume::CACHE_FOR_READ);
  if (!p) return false;

  // format name
  dirName(*p, name);
  return true;
}

//------------------------------------------------------------------------------
/** Read the next directory entry from a directory file.
 *
 * \param[out] dir The dir_t struct that will receive the data.
 *
 * \return For success readDir() returns the number of bytes read.
 * A value of zero will be returned if end of file is reached.
 * If an error occurs, readDir() returns -1.  Possible errors include
 * readDir() called before a directory has been opened, this is not
 * a directory file or an I/O error occurred.
 */
int16_t SdFileExt::readDir(dir_t* dir, char* longFilename) {
  int16_t n;
  // if not a directory file or miss-positioned return an error
  if (!isDir() || (0X1F & curPosition_)) return -1;

  //If we have a longFilename buffer, mark it as invalid. If we find a long filename it will be filled automaticly.
  if (longFilename != NULL)
  {
     memset( longFilename, '\0', LONG_FILENAME_LENGTH );
  }

  while (1) {
    n = read(dir, sizeof(dir_t));
    if (n != sizeof(dir_t)) return n == 0 ? 0 : -1;
    // last entry if DIR_NAME_FREE
    if (dir->name[0] == DIR_NAME_FREE) return 0;
    // skip empty entries and entry for .  and ..
    if (dir->name[0] == DIR_NAME_DELETED || dir->name[0] == '.') continue;
    //Fill the long filename if we have a long filename entry,
	// long filename entries are stored before the actual filename.
	if (DIR_IS_LONG_NAME(dir) && longFilename != NULL)
    {
    	vfat_t *VFAT = (vfat_t*)dir;
    	uint8_t blocknr = (VFAT->sequenceNumber & 0x1F);
		//Sanity check the VFAT entry. The first cluster is always set to zero. And th esequence number should be higher then 0
    	if ((VFAT->firstClusterLow == 0) && (blocknr > 0) && (blocknr <= MAX_VFAT_ENTRIES))
    	{
			//TODO: Store the filename checksum to verify if a none-long filename aware system modified the file table.
    		uint8_t i = (blocknr - 1) * 13;
			longFilename[i++] = VFAT->name1[0];
			longFilename[i++] = VFAT->name1[2];
			longFilename[i++] = VFAT->name1[4];
			longFilename[i++] = VFAT->name1[6];
			longFilename[i++] = VFAT->name1[8];
			longFilename[i++] = VFAT->name2[0];
			longFilename[i++] = VFAT->name2[2];
			longFilename[i++] = VFAT->name2[4];
			longFilename[i++] = VFAT->name2[6];
			longFilename[i++] = VFAT->name2[8];
			longFilename[i++] = VFAT->name2[10];
			longFilename[i++] = VFAT->name3[0];
			longFilename[i++] = VFAT->name3[2];
			//If this VFAT entry is the last one, add a NUL terminator at the end of the string
			if (VFAT->sequenceNumber & 0x40)
              longFilename[i] = '\0';

//    SERIAL_ECHO_START;
//    SERIAL_ECHOPAIR("longFilename index=", (long unsigned int)(i+13) );
//    SERIAL_ECHOLN("");

		}
    }
    // return if normal file or subdirectory
    if (DIR_IS_FILE_OR_SUBDIR(dir)) return n;
  }
}


#endif // SDSUPPORT
