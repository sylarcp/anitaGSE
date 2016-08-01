// Implementation of file handler

#include <iostream>
#include <fstream>
using namespace::std;
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "fileHandler.h"
extern "C"{
#include <stdlib.h>
}

fileHandler::fileHandler(const char *dir_p){
  if(dir_p){
    basedir=dir_p;
    basedir+='/';
  }

  // Make directory structure needed if already not present
  for(int i=0;i<N_PCKTYPE;++i){
    string dir=typeDir(i);
    makeDir(dir);
  }
   
  // Try opening already existing files 
  for(int i=0;i<N_PCKTYPE;++i){
    switch(i){
    case TYPE_WV:
    case TYPE_PEDSUB_WV:
    case TYPE_ENC_WV_PEDSUB:
    case TYPE_SURF:
    case TYPE_PEDSUB_SURF:
    case TYPE_ENC_SURF:
    case TYPE_ENC_SURF_PEDSUB:
    case TYPE_LAB_PED:
    case TYPE_ZIPPED_FILE: // No for wv, pswv, peds, and config dumps
      fp[i]=NULL;
      continue;
    default:{
      string dir=typeDir(i);
      if(dir.length()){
	string curfile=dir+"current";
	ifstream ifile(curfile.c_str());
	if(ifile.is_open()){
	  string name;
	  ifile >> currentFile[i] >> currentCount[i];
	  name=dir+currentFile[i];
	  fp[i]=fopen(name.c_str(),"a");
	  ifile.close();
	}else{
	  fp[i]=NULL;
	  currentCount[i]=0;
	}
      }
    }
    }
  }
}

fileHandler::~fileHandler(){
  // Close all files and record for next run
  for(int i=0;i<N_PCKTYPE;++i){
    if(fp[i]) fclose(fp[i]);
    switch(i){
    case TYPE_WV:
    case TYPE_PEDSUB_WV:
    case TYPE_ENC_WV_PEDSUB:
    case TYPE_SURF:
    case TYPE_PEDSUB_SURF:
    case TYPE_ENC_SURF:
    case TYPE_ENC_SURF_PEDSUB:
    case TYPE_LAB_PED:
    case TYPE_ZIPPED_FILE: // No for wv, pswv, peds, and config dumps
      continue;
    default:{
      string dir=typeDir(i);
      if(dir.length() && fp[i]!=NULL){
	string curfile=dir+"current";
	ofstream ofile(curfile.c_str());
	if(ofile.is_open()){
	  ofile << currentFile[i] << " " <<  currentCount[i];
	  ofile.close();
	}
      }
    }
    }
  }
}

void fileHandler::makeDir(const string dir){
  if(dir.length()){
    int p=basedir.length()-1;
    while((p=dir.find('/',p+1))!=string::npos){
      mkdir(dir.substr(0,p).c_str(),0777);
    }
  }
}

string fileHandler::typeDir(int type){
  string dir;
  const char *dir_p=pckDirbase(type);
  if(dir_p) dir=basedir+dir_p;
  return dir;
}

bool fileHandler::openNewFile(int type,const void *pckStruct_p){
  string dir=typeDir(type);
  const char *head=pckFilebase(type);
  if(!dir.length() || !head) return false;

  // Generate file name
  ostringstream namestream;
  namestream << string(head) << '_';
  long tmp;
  if(tmp=pckEventNumber(pckStruct_p))
    namestream << tmp;
  else if(tmp=pckTime(pckStruct_p)){
    namestream << tmp;
    //    if(tmp=pckSubTime(pckStruct_p)){
    //      namestream << '_' << tmp;
    //    }
  }else
    return false;
  namestream << ".dat";
  string name=namestream.str();
  currentFile[type]=name;
  name.insert(0,dir);

  fp[type]=fopen(name.c_str(),"wb");
  return fp[type]!=NULL;
}

bool fileHandler::openTextFile(int type,const void *pckStruct_p){
  string dir=typeDir(type);
  const char *head=pckFilename(pckStruct_p);
  if(!dir.length() || !head) return false;

  // Generate file name
  ostringstream namestream;
  namestream << string(head) << '_';
  long tmp;
  if(tmp=pckTime(pckStruct_p)){
    namestream << tmp;
  }else
    return false;
  string name=namestream.str();
  currentFile[type]=name;
  name.insert(0,dir);

  if(fp[type]) fclose(fp[type]);
  fp[type]=fopen(name.c_str(),"w");
  return fp[type]!=NULL;
}

bool fileHandler::openPedestalFile(int type,const void *pckStruct_p,const char *fmt){
  long tmp;
  if(!(tmp=pckTime(pckStruct_p))) return false;
  
  if(fp[type]) fclose(fp[type]);

  string dir=typeDir(type);
  const char *head=pckFilebase(type);
  if(!dir.length() || !head) return false;
  
  // Generate file name
  ostringstream namestream;
  namestream << string(head) << '_' << tmp << ".dat";
  string name=namestream.str();
  name.insert(0,dir);
  
  fp[type]=fopen(name.c_str(),fmt); 
  return fp[type]!=NULL;
}

bool fileHandler::openWaveformFile(int type,const void *pckStruct_p){
  static long currOpenEvent=-1;
  long tmp;

  if(tmp=pckEventNumber(pckStruct_p)){
    if(tmp==currOpenEvent) return true;
  }else
    return false;

  string dir=typeDir(type);
  const char *head=pckFilebase(type);
  if(!dir.length() || !head) return false;

  // Make directory structure needed
  int topnum=(tmp/100000)*100000;
  int subnum=(tmp/1000)*1000;
  ostringstream namestream;
  namestream << "ev_" << topnum << "/ev_" << subnum << '/'; 
  string subdir=namestream.str();
  subdir.insert(0,dir);
  makeDir(subdir);

  // Generate file name
  namestream << string(head) << '_' << tmp << ".dat";
  string name=namestream.str();
  currentFile[type]=name;
  name.insert(0,dir);

  // Close current file
  if(fp[type]) fclose(fp[type]);
  currOpenEvent=-1;

  fp[type]=fopen(name.c_str(),"ab");
  if(!fp[type]) return false;
  
  currOpenEvent=tmp;
  currentCount[type]=0;
  return true;
}

bool fileHandler::openHKCalibFile(int type,const void *pckStruct_p){
  int tmptype=TYPE_ZIPPED_PACKET;  // Reuse type of unsupported zipped packets
  long tmp;
  if(!(tmp=pckTime(pckStruct_p))) return false;

  string dir=typeDir(type);
  const char *head=pckFilebase(type);
  if(!dir.length() || !head) return false;
  
  // Generate file name
  ostringstream namestream;
  string headstr=head;
  headstr.replace(headstr.find("raw"),3,"cal_avz");;
  namestream << headstr << '_' << tmp << ".dat";
  string name=namestream.str();
  name.insert(0,dir);

  fp[tmptype]=fopen(name.c_str(),"ab");
  return fp[tmptype]!=NULL;
}

bool fileHandler::store(const void *pckStruct_p){
  int type=pckType(pckStruct_p);
  int size=pckSize(pckStruct_p);
  if(type>=N_PCKTYPE || !size) return false;
  
  switch(type){
  case TYPE_WV:
  case TYPE_SURF:
  case TYPE_ENC_SURF:{ // Change size to correct size of the packet sent
    PacketCode_t code=PACKET_WV;
    size=pckSize(&code);
    if(!openWaveformFile(type,pckStruct_p)) return false;
    break;
  }
  case TYPE_PEDSUB_WV:
  case TYPE_ENC_WV_PEDSUB:
  case TYPE_PEDSUB_SURF:
  case TYPE_ENC_SURF_PEDSUB:{
    // Change size to correct size of the packet sent
    PacketCode_t code=PACKET_PEDSUB_WV;
    size=pckSize(&code);
    if(!openWaveformFile(type,pckStruct_p)) return false;
    break;
  }
  case TYPE_LAB_PED: // Pedestals handled differently   
    return storePedestal(pckStruct_p);
  case TYPE_ZIPPED_FILE: // Config file dumps handled differently
    return storeTextFile(pckStruct_p);
  case TYPE_HKD:
    if(pckHKAnalogueCode(pckStruct_p)!=IP320_RAW){ // Not IP320_RAW (i.e. calib), then open different file
      if(!openHKCalibFile(type,pckStruct_p)) return false;
      type=TYPE_ZIPPED_PACKET; // Using empty spot since zipped packets will not be written out directly
      break;
    }
    // Otherwise drop through for normal handling
  default:
    if(!fp[type] && !openNewFile(type,pckStruct_p)) return false;
  }

  if(fwrite(const_cast<void*>(pckStruct_p),size,1,fp[type])!=1) return false;

  /* Close and zip files; waveforms with 81 packets, header files with 100 packets 
     and others with 1000, or HK calib files always (but no zipping). 
     Ped files, config dumps, and wake-ups don't reach this point. */
  ++currentCount[type];
  if((type==TYPE_HD && currentCount[type]==EVENTS_PER_FILE) || 
     ((type==TYPE_WV            ||
       type==TYPE_PEDSUB_WV     ||
       type==TYPE_ENC_WV_PEDSUB ||
       type==TYPE_SURF          ||
       type==TYPE_PEDSUB_SURF   ||
       type==TYPE_ENC_SURF      ||
       type==TYPE_ENC_SURF_PEDSUB) && currentCount[type]==ACTIVE_SURFS*CHANNELS_PER_SURF) ||  
     ((type==TYPE_SURF_HK       ||
       type==TYPE_TURF_RATE     ||
       type==TYPE_GPS_ADU5_PAT  ||
       type==TYPE_GPS_ADU5_VTG  ||
       type==TYPE_GPS_ADU5_SAT  ||
       type==TYPE_GPS_G12_POS   ||
       type==TYPE_GPS_G12_SAT   ||
       type==TYPE_HKD           ||
       type==TYPE_CMD_ECHO      ||
       type==TYPE_MONITOR       ||
       type==TYPE_SLOW1         ||
       type==TYPE_SLOW2) && currentCount[type]==HK_PER_FILE) || 
     type==TYPE_ZIPPED_PACKET){
    fclose(fp[type]);
    fp[type]=NULL;
    currentCount[type]=0;
    if(type!=TYPE_ZIPPED_PACKET){
      string name=basedir+pckDirbase(type)+currentFile[type];
      string cmd="gzip -f -q "+name;
      system(cmd.c_str());
    }
  }
  return true;
}

bool fileHandler::storePedestal(const void *pckStruct_p){
  static int dummyCount=0;
  PedestalStruct_t ped;
  
  // This routine will be called CHANNELS_PER_SURF times per each packet, so store 
  // data only on the first invocation
  if(dummyCount++%CHANNELS_PER_SURF!=0) return true; 

  int type=pckType(pckStruct_p);
  int size=sizeof(PedestalStruct_t);

  // Try opening pedestal file for reading 
  bool read=false;
  if(openPedestalFile(type,pckStruct_p,"rb")){
    // Try reading alredy output pedestal data
    if(!(read=fread(&ped,size,1,fp[type]))) return false;
  }

  // Insert new data into output structure
  FullLabChipPedStruct_t *ped_p=(FullLabChipPedStruct_t*)pckStruct_p;
  int nChip=ped_p->pedChan[0].chipId;
  if(!read){
    ped.unixTime=pckTime(pckStruct_p);
    ped.nsamples=ped_p->pedChan[0].chipEntries;
  }
  for(int j=0;j<CHANNELS_PER_SURF;++j){
    int nSurf=ped_p->pedChan[j].chanId/ACTIVE_SURFS;
    int nChan=ped_p->pedChan[j].chanId%ACTIVE_SURFS;
    if(nSurf<0 || nSurf>ACTIVE_SURFS-1){ cerr << "storePedestal: Bad nSurf " << nSurf << endl; continue;}
    if(nChan<0 || nChan>CHANNELS_PER_SURF-1){ cerr << "storePedestal: Bad nChan " << nChan << endl; continue;}
    for(int i=0;i<MAX_NUMBER_SAMPLES;++i){ 
      ped.thePeds[nSurf][nChip][nChan][i]=ped_p->pedChan[j].pedMean[i]/10; //  PedestalStruct_t has pedestals in x1 form, not x10 as FullLabChipPedStruct_t
      ped.pedsRMS[nSurf][nChip][nChan][i]=ped_p->pedChan[j].pedRMS[i]; 
    }
  }
  
  // Output updated structure
  if(openPedestalFile(type,pckStruct_p,"wb")){
    if(fwrite(&ped,size,1,fp[type])!=1) return false;
  }else
    return false;
  // Close pedestal file
  fclose(fp[type]);
  fp[type]=NULL;
  return true;
}
  
bool fileHandler::storeTextFile(const void *pckStruct_p){
  int type=pckType(pckStruct_p);

  // Open file for output
  if(!openTextFile(type,pckStruct_p)) return false;

  ZippedFile_t *zip_p=(ZippedFile_t*)pckStruct_p;
  int unzippedBytes=zip_p->numUncompressedBytes;
  int zippedBytes=zip_p->gHdr.numBytes-sizeof(ZippedFile_t);
  char *file=new char[unzippedBytes];
  int retVal=uncompress((Bytef*)file,(uLongf*)&unzippedBytes,(const Bytef*)((const char*)zip_p+sizeof(ZippedFile_t)),zippedBytes);
  if(retVal!=Z_OK || unzippedBytes!=zip_p->numUncompressedBytes) return false;

  // Output uncompressed file content 
  if(fwrite(file,sizeof(char),unzippedBytes,fp[type])!=unzippedBytes) return false;
  // Close output file
  fclose(fp[type]);
  fp[type]=NULL;
  // Compress the output file
  string name=basedir+pckDirbase(type)+currentFile[type];
  string cmd="gzip -q -f "+name;
  system(cmd.c_str());

  return true;
}
  
  
  
