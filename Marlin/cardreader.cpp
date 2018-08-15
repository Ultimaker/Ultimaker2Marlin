#include "Marlin.h"
#include "cardreader.h"
#include "UltiLCD2.h"
#include "ultralcd.h"
#include "stepper.h"
#include "temperature.h"
#include "language.h"

#ifdef SDSUPPORT

CardReader::CardReader() :
   state(SD_CHECKAUTOSTART)
 , workDirDepth(0)
 , insertChangeDelay(1000 / 25)
 , filesize(0)
 , autostart_atmillis(0)
 , sdpos(0)
{
  //power to SD reader
  #if SDPOWER > -1
    SET_OUTPUT(SDPOWER);
    WRITE(SDPOWER,HIGH);
  #endif //SDPOWER

  autostart_atmillis=millis()+5000;
}

char *createFilename(char *buffer, const dir_t &p) //buffer>12characters
{
  char *pos=buffer;
  for (uint8_t i = 0; i < 11; ++i)
  {
    if (p.name[i] == ' ')continue;
    if (i == 8)
    {
      *pos++='.';
    }
    *pos++=p.name[i];
  }
  *pos=0;
  return buffer;
}


void  CardReader::lsDive(SdFile &parent, SdFile** parents, uint8_t dirDepth)
{
  dir_t p;
  uint8_t cnt=0;

  while (parent.readDir(&p, longFilename) > 0)
  {
    if( DIR_IS_SUBDIR(&p) && (lsAction!=LS_Count) && (lsAction!=LS_GetFilename)) // hence LS_SerialPrint
    {

      char lfilename[13];
      createFilename(lfilename, p);

      SdFile dir;

      if(!dir.open(parent, lfilename, O_READ))
      {
        if(lsAction==LS_SerialPrint)
        {
          SERIAL_ECHO_START;
          SERIAL_ECHOLNPGM(MSG_SD_CANT_OPEN_SUBDIR);
          SERIAL_ECHOLN(lfilename);
        }
      }
      else if (parents && (dirDepth < MAX_DIR_DEPTH)) {
        parents[dirDepth] = &dir;
        lsDive(dir, parents, dirDepth+1);
      }
      //close done automatically by destructor of SdFile

    }
    else
    {
      if (p.name[0] == DIR_NAME_FREE) break;
      if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.'|| p.name[0] == '_') continue;
      if (longFilename[0] != '\0' &&
          (longFilename[0] == '.' || longFilename[0] == '_')) continue;
      if ( p.name[0] == '.')
      {
        if ( p.name[1] != '.')
        continue;
      }

      if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;
      if (DIR_IS_SUBDIR(&p)) {
        state |= SD_ISDIR;
      }
      else {
        state &= ~SD_ISDIR;
        if(p.name[8]!='G') continue;
        if(p.name[9]=='~') continue;
      }

      createFilename(filename,p);
      if(lsAction==LS_SerialPrint)
      {
        char lfilename[13];
        SERIAL_PROTOCOLPGM("/");
        for (uint8_t d=0; d<dirDepth; ++d) {
          if (parents[d]->getFilename(lfilename)) {
            SERIAL_PROTOCOL(lfilename);
            SERIAL_PROTOCOLPGM("/");
          }
          else {
            break;
          }
        }
        SERIAL_PROTOCOLLN(filename);
      }
      else if(lsAction==LS_Count)
      {
        nrFiles++;
      }
      else if(lsAction==LS_GetFilename)
      {
        if(cnt==nrFiles)
          return;
        cnt++;
      }
    }
  }
}

void CardReader::ls()
{
  lsAction=LS_SerialPrint;
  root.rewind();
  SdFile* lsParents[MAX_DIR_DEPTH] = {0};
  lsDive(root, lsParents, 0);
}


void CardReader::initsd()
{
  // cardOK = false;
  state &= ~SD_OK;
  if(root.isOpen())
    root.close();
#ifdef SDSLOW
  if (!card.init(SPI_HALF_SPEED,SDSS))
#else
  if (!card.init(SPI_FULL_SPEED,SDSS))
#endif
  {
    //if (!card.init(SPI_HALF_SPEED,SDSS))
    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM(MSG_SD_INIT_FAIL);
  }
  else if (!volume.init(&card))
  {
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_SD_VOL_INIT_FAIL);
  }
  else if (!root.openRoot(&volume))
  {
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_SD_OPENROOT_FAIL);
  }
  else
  {
    // cardOK = true;
    state |= SD_OK;
    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM(MSG_SD_CARD_OK);
  }
  workDir=root;
  curDir=&root;
  /*
  if(!workDir.openRoot(&volume))
  {
    SERIAL_ECHOLNPGM(MSG_SD_WORKDIR_FAIL);
  }
  */

}

void CardReader::setroot()
{
  /*if(!workDir.openRoot(&volume))
  {
    SERIAL_ECHOLNPGM(MSG_SD_WORKDIR_FAIL);
  }*/
  workDir=root;
  curDir=&workDir;
  workDirDepth = 0;
}

void CardReader::release()
{
//  sdprinting = false;
//  pause = false;
//  cardOK = false;
  state &= ~(SD_PRINTING | SD_PAUSE | SD_OK);
}

void CardReader::startFileprint()
{
  if(cardOK())
  {
//    sdprinting = true;
//    pause = false;
    state |= SD_PRINTING;
    state &= ~SD_PAUSE;
  }
}

void CardReader::openLogFile(const char* name)
{
  state |= SD_LOGGING;
  openFile(name, false);
}

void CardReader::openFile(const char* name,bool read)
{
  if(!cardOK())
    return;
  file.close();
//  sdprinting = false;
//  pause = false;
  state &= ~(SD_PRINTING | SD_PAUSE | SD_SAVING | SD_LOGGING);

  SdFile myDir;
  curDir=&root;
  const char *fname=name;

  char *dirname_start,*dirname_end;
  if(name[0]=='/')
  {
    dirname_start=strchr(name,'/')+1;
    while(dirname_start>(char*)1)
    {
      dirname_end=strchr(dirname_start,'/');
      if(dirname_end>0 && dirname_end>dirname_start)
      {
        char subdirname[13];
        strncpy(subdirname, dirname_start, dirname_end-dirname_start);
        subdirname[dirname_end-dirname_start]=0;
        SERIAL_ECHOLN(subdirname);
        if(!myDir.open(curDir,subdirname,O_READ))
        {
          SERIAL_PROTOCOLPGM(MSG_SD_OPEN_FILE_FAIL);
          SERIAL_PROTOCOL(subdirname);
          SERIAL_PROTOCOLLNPGM(".");
          return;
        }

        curDir=&myDir;
        dirname_start=dirname_end+1;
      }
      else // the reminder after all /fsa/fdsa/ is the filename
      {
        fname=dirname_start;
        break;
      }
    }
  }
  else //relative path
  {
    curDir=&workDir;
  }
  if(read)
  {
    if (file.open(curDir, fname, O_READ))
    {
      filesize = file.fileSize();
      SERIAL_PROTOCOLPGM(MSG_SD_FILE_OPENED);
      SERIAL_PROTOCOL(fname);
      SERIAL_PROTOCOLPGM(MSG_SD_SIZE);
      SERIAL_PROTOCOLLN(filesize);
      sdpos = 0;

      SERIAL_PROTOCOLLNPGM(MSG_SD_FILE_SELECTED);
#ifndef ENABLE_ULTILCD2
       lcd_setstatus(fname);
#endif
    }
    else
    {
      SERIAL_PROTOCOLPGM(MSG_SD_OPEN_FILE_FAIL);
      SERIAL_PROTOCOL(fname);
      SERIAL_PROTOCOLLNPGM(".");
    }
  }
  else
  { //write
    if (!file.open(curDir, fname, O_CREAT | O_APPEND | O_WRITE | O_TRUNC))
    {
      SERIAL_PROTOCOLPGM(MSG_SD_OPEN_FILE_FAIL);
      SERIAL_PROTOCOL(fname);
      SERIAL_PROTOCOLLNPGM(".");
    }
    else
    {
      state |= SD_SAVING;
      SERIAL_PROTOCOLPGM(MSG_SD_WRITE_TO_FILE);
      SERIAL_PROTOCOLLN(name);
#ifndef ENABLE_ULTILCD2
      lcd_setstatus(fname);
#endif
    }
  }
}

void CardReader::removeFile(const char* name)
{
  if(!cardOK())
    return;
  file.close();
//  sdprinting = false;
//  pause = false;
  state &= ~(SD_PRINTING | SD_PAUSE | SD_SAVING | SD_LOGGING);

  SdFile myDir;
  curDir=&root;
  const char *fname=name;

  char *dirname_start,*dirname_end;
  if(name[0]=='/')
  {
    dirname_start=strchr(name,'/')+1;
    while(dirname_start>0)
    {
      dirname_end=strchr(dirname_start,'/');
      if(dirname_end>0 && dirname_end>dirname_start)
      {
        char subdirname[13];
        strncpy(subdirname, dirname_start, dirname_end-dirname_start);
        subdirname[dirname_end-dirname_start]=0;
        SERIAL_ECHOLN(subdirname);
        if(!myDir.open(curDir,subdirname,O_READ))
        {
          SERIAL_PROTOCOLPGM("open failed, File: ");
          SERIAL_PROTOCOL(subdirname);
          SERIAL_PROTOCOLLNPGM(".");
          return;
        }

        curDir=&myDir;
        dirname_start=dirname_end+1;
      }
      else // the remainder after all /fsa/fdsa/ is the filename
      {
        fname=dirname_start;
        break;
      }
    }
  }
  else //relative path
  {
    curDir=&workDir;
  }
    if (file.remove(curDir, fname))
    {
      SERIAL_PROTOCOLPGM("File deleted:");
      SERIAL_PROTOCOL(fname);
      sdpos = 0;
    }
    else
    {
      SERIAL_PROTOCOLPGM("Deletion failed, File: ");
      SERIAL_PROTOCOL(fname);
      SERIAL_PROTOCOLLNPGM(".");
    }
}

void CardReader::getStatus()
{
  if(cardOK()){
    SERIAL_PROTOCOLPGM(MSG_SD_PRINTING_BYTE);
    SERIAL_PROTOCOL(sdpos);
    SERIAL_PROTOCOLPGM("/");
    SERIAL_PROTOCOLLN(filesize);
  }
  else{
    SERIAL_PROTOCOLLNPGM(MSG_SD_NOT_PRINTING);
  }
  if (card.errorCode())
  {
    SERIAL_PROTOCOLPGM("Card error:");
    SERIAL_PROTOCOLLN(card.errorCode());
  }
}

void CardReader::write_command(char *buf)
{
  char* begin = buf;
  char* npos = 0;
  char* end = buf + strlen(buf) - 1;

  file.clearWriteError();
  if((npos = strchr(buf, 'N')) != NULL)
  {
    begin = strchr(npos, ' ') + 1;
    end = strchr(npos, '*') - 1;
  }
  end[1] = '\r';
  end[2] = '\n';
  end[3] = '\0';
  file.write(begin);
  if (file.getWriteError())
  {
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_SD_ERR_WRITE_TO_FILE);
  }
}

bool CardReader::write_string(char* buffer)
{
    file.write(buffer);
    return file.getWriteError();
}

void CardReader::checkautostart(bool force)
{
  static uint8_t lastnr = 0;
  if(!force)
  {
    if(!(state & SD_CHECKAUTOSTART))
      return;
    if(autostart_atmillis<millis())
      return;
    lastnr = 0;
  }
  // autostart_stilltocheck=false;
  state &= ~SD_CHECKAUTOSTART;
  if (lastnr > 9)
  {
      return;
  }

  if(!cardOK())
  {
    initsd();
    if(!cardOK()) //fail
      return;
  }

  char autoname[30];
  sprintf_P(autoname, PSTR("auto%i.g"), lastnr);

  root.rewind();

  bool found=false;
  {
    dir_t p;
    while (root.readDir(&p, NULL) > 0)
    {
      // convert filename to lowercase
      uint8_t *pChar = p.name;
      for(uint8_t i=0; i<sizeof(p.name); ++i, ++pChar)
        *pChar = tolower(*pChar);

      if(p.name[9]!='~') //skip safety copies
      if(strncmp((char*)p.name,autoname,5)==0)
      {
        char cmd[16];

        sprintf_P(cmd, PSTR("M23 %s"), autoname);
        enquecommand(cmd);
        enquecommand_P(PSTR("M24"));
        found=true;
      }
    }
  }
  if(!found)
  {
    lastnr=0xFF;
  }
  else
  {
    lastnr++;
  }
  clearError();
}

void CardReader::closefile()
{
  file.sync();
  file.close();
//  saving = false;
//  logging = false;
  state &= ~(SD_SAVING | SD_LOGGING);
}

void CardReader::getfilename(const uint8_t nr)
{
  curDir=&workDir;
  lsAction=LS_GetFilename;
  nrFiles=nr;
  curDir->rewind();
  lsDive(*curDir, NULL, 0);
}

uint16_t CardReader::getnrfilenames()
{
  curDir=&workDir;
  lsAction=LS_Count;
  nrFiles=0;
  curDir->rewind();
  lsDive(*curDir, NULL, 0);
  //SERIAL_ECHOLN(nrFiles);
  return nrFiles;
}

void CardReader::chdir(const char * relpath)
{
  SdFile newfile;
  SdFile *parent=&root;

  if(workDir.isOpen())
    parent=&workDir;

  if(!newfile.open(*parent,relpath, O_READ))
  {
   SERIAL_ECHO_START;
   SERIAL_ECHOPGM(MSG_SD_CANT_ENTER_SUBDIR);
   SERIAL_ECHOLN(relpath);
  }
  else
  {
    if (workDirDepth < MAX_DIR_DEPTH) {
      for (int d = ++workDirDepth; d--;)
        workDirParents[d+1] = workDirParents[d];
      workDirParents[0]=*parent;
    }
    workDir=newfile;
  }
}

void CardReader::updir()
{
  if(workDirDepth > 0)
  {
    --workDirDepth;
    workDir = workDirParents[0];
    for (uint8_t d = 0; d < workDirDepth; d++)
      workDirParents[d] = workDirParents[d+1];
  }
}

void CardReader::printingHasFinished()
{
    st_synchronize();
    quickStop();
    file.close();
//    sdprinting = false;
//    pause = false;
    state &= ~(SD_PRINTING | SD_PAUSE | SD_SAVING | SD_LOGGING);
    if(SD_FINISHED_STEPPERRELEASE)
    {
        //finishAndDisableSteppers();
        enquecommand_P(PSTR(SD_FINISHED_RELEASECOMMAND));
    }
    autotempShutdown();
}

void CardReader::getFilenameFromNr(char* buffer, uint8_t nr)
{
	getfilename(nr);
	if (*longFilename)
	{
		strncpy(buffer, longFilename, LONG_FILENAME_LENGTH-1);
	}
	else
    {
		strncpy(buffer, filename, LONG_FILENAME_LENGTH-1);
	}
	if (!filenameIsDir())
	{
		if (strrchr(buffer, '.')) strrchr(buffer, '.')[0] = '\0';
	}
}

#endif //SDSUPPORT
