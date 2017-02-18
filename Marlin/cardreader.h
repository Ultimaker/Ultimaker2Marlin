#ifndef CARDREADER_H
#define CARDREADER_H

#ifdef SDSUPPORT

#define MAX_DIR_DEPTH 10

#if (SDCARDDETECT > -1)
# ifdef SDCARDDETECTINVERTED
#  define IS_SD_INSERTED (READ(SDCARDDETECT)!=0)
# else
#  define IS_SD_INSERTED (READ(SDCARDDETECT)==0)
# endif //SDCARDTETECTINVERTED
#else
//If we don't have a card detect line, always assume the card is inserted
# define IS_SD_INSERTED true
#endif

#include "SdFile.h"

#define SD_OK               1
#define SD_SAVING           2
#define SD_LOGGING          4
#define SD_PRINTING         8
#define SD_PAUSE           16
#define SD_INSERTED        32
#define SD_ISDIR           64
#define SD_CHECKAUTOSTART 128

enum LsAction {LS_SerialPrint,LS_Count,LS_GetFilename};

class CardReader
{
public:
  CardReader();

  void initsd();
  void write_command(char *buf);
  bool write_string(char* buffer);
  //files auto[0-9].g on the sd card are performed in a row
  //this is to delay autostart and hence the initialization of the sd card to some seconds after the normal init, so the device is available quick after a reset

  void checkautostart(bool force);
  void openFile(const char* name,bool read);
  void openLogFile(const char* name);
  void removeFile(const char* name);
  void closefile();
  void release();
  void startFileprint();
  void getStatus();
  void printingHasFinished();

  void getfilename(const uint8_t nr);
  void getFilenameFromNr(char* buffer, uint8_t nr);
  uint16_t getnrfilenames();


  void ls();
  void chdir(const char * relpath);
  void updir();
  void setroot();


  FORCE_INLINE bool isFileOpen() { return file.isOpen(); }
  FORCE_INLINE bool eof() { return sdpos>=filesize ;}
  FORCE_INLINE int16_t get() {  sdpos = file.curPosition();return (int16_t)file.read();}
  FORCE_INLINE int16_t fgets(char* str, int16_t num) { return file.fgets(str, num, NULL); }
  FORCE_INLINE void setIndex(long index) {sdpos = index;file.seekSet(index);}
  FORCE_INLINE uint8_t percentDone(){if(!isFileOpen()) return 0; if(filesize) return sdpos/((filesize+99)/100); else return 0;}
  FORCE_INLINE char* getWorkDirName(){workDir.getFilename(filename);return filename;}
  FORCE_INLINE bool atRoot() { return workDirDepth==0; }
  FORCE_INLINE uint32_t getFilePos() { return sdpos; }
  FORCE_INLINE uint32_t getFileSize() { return filesize; }
  FORCE_INLINE bool isOk() { return cardOK() && card.errorCode() == 0; }
  FORCE_INLINE int errorCode() { return card.errorCode(); }
  FORCE_INLINE void clearError() { card.clearError(); }
  FORCE_INLINE void updateSDInserted()
  {
    bool newInserted = IS_SD_INSERTED;
    if (sdInserted() != newInserted)
    {
      if (insertChangeDelay)
        insertChangeDelay--;
      else{
        if (newInserted)
          state |= SD_INSERTED;
        else
          state &= ~SD_INSERTED;
      }
    }else{
      insertChangeDelay = 1000 / 25;
    }
  }

  FORCE_INLINE const char * currentFileName() const { return filename; }
  FORCE_INLINE const char * currentLongFileName() const { return longFilename; }
  FORCE_INLINE void setLongFilename(const char *name) { strncpy(longFilename, name, LONG_FILENAME_LENGTH-1); }
  FORCE_INLINE void truncateLongFilename(uint8_t pos) { longFilename[pos] = '\0'; if (char *point = strchr(longFilename, '.')) *point = '\0'; }
  FORCE_INLINE void clearLongFilename() { longFilename[0] = '\0'; }

  FORCE_INLINE bool cardOK() const { return state & SD_OK; }
  FORCE_INLINE bool sdprinting() const { return state & SD_PRINTING; }
  FORCE_INLINE bool saving() const { return state & SD_SAVING; }
  FORCE_INLINE bool logging() const { return state & SD_LOGGING; }
  FORCE_INLINE bool pause() const { return state & SD_PAUSE; }
  FORCE_INLINE bool sdInserted() const { return state & SD_INSERTED; }
  FORCE_INLINE bool filenameIsDir() const { return state & SD_ISDIR; }

  FORCE_INLINE void stopPrinting() { state &= ~(SD_PRINTING | SD_PAUSE); }
  FORCE_INLINE void pauseSDPrint() { state |= SD_PAUSE; }
  FORCE_INLINE void resumePrinting() { state &= ~SD_PAUSE; }

private:
  char filename[13];
  char longFilename[LONG_FILENAME_LENGTH];

  uint8_t state;
  SdFile root,*curDir,workDir,workDirParents[MAX_DIR_DEPTH];
  uint8_t workDirDepth;
  uint8_t insertChangeDelay;
  Sd2Card card;
  SdVolume volume;
  SdFile file;
  uint32_t filesize;
  unsigned long autostart_atmillis;
  uint32_t sdpos ;

  LsAction lsAction; //stored for recursion.
  int16_t nrFiles; //counter for the files in the current directory and recycled as position counter for getting the nrFiles'th name in the directory.
  char* diveDirName;
  void lsDive(SdFile &parent, SdFile** parents, uint8_t dirDepth);
};
extern CardReader card;
#define IS_SD_PRINTING (card.sdprinting())

#else

#define IS_SD_PRINTING (false)

#endif //SDSUPPORT
#endif
