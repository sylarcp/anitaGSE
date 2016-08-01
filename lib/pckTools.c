/*  Copyright (C) 2005 Predrag Miocinovic <predrag@phys.hawaii.edu> 

  This file is free software; as a special exception the author gives
  unlimited permission to copy and/or distribute it, with or without 
  modifications, as long as this notice is preserved. 

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY, to the extent permitted by law; without even the 
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Functions for manipulating GSE data packets

  $Header$
*/

#include <stdio.h>
#include <math.h>

#include "includes/anitaStructures.h"
#include "anitaGSE.h"

// Verify full sanity of LOS data packet w/o CRC cheksum check
// This is check for los_2006b format
int goodLOS(unsigned char *pck,int n){
//not in use in anita3.  Raw_los data use goodSiphr
  int good=1;
  short nData;
  // Leading and trailing wrapper words  
  good&=(*((unsigned short*)pck)==0xF00D);
  good&=(*((unsigned short*)(pck+6))==0xF00D);
  good&=(*((unsigned short*)(pck+8))==0xD0CC);
  good&=(*((unsigned short*)(pck+10))>=0xAE00 && 
	 *((unsigned short*)(pck+10))<=0xAE03);
  good&=(*((unsigned short*)(pck+n-6))==0xAEFF);
  good&=(*((unsigned short*)(pck+n-4))==0xC0FE);
  good&=( (*((unsigned short*)(pck+n-2))==0xD0CC) || (*((unsigned short*)(pck+n-2))==0xF1EA) );
   
  // Length of packet and declared length of content match
  nData = getLSBshort(pck+16);
  good&=(n==nData+26);

  return good;
}

// Verify full sanity of high speed TDRSS data packet w/o CRC cheksum check
// This is check for sip_hr_2006a format
int goodSIPhr(unsigned char *pck,int n){
  int good=1;
  short nData;
//printf("begin goodSiphr! n = %d\n", n);
  // Leading and trailing wrapper words
  good&=(*((unsigned short*)pck)==0xF00D);
//printf(" goodSiphr %d\n", good);
  good&=(*((unsigned short*)(pck+2))==0xD0CC);
//printf(" goodSiphr %d\n", good);
  good&=(*((unsigned short*)(pck+4))>=0xAE00 && 
	 *((unsigned short*)(pck+4))<=0xAE03);
//printf(" goodSiphr %d\n", good);
  good&=(*((unsigned short*)(pck+n-6))==0xAEFF);
//printf(" goodSiphr %d\n", good);
  good&=(*((unsigned short*)(pck+n-4))==0xC0FE);
//printf(" goodSiphr %d\n", good);
  good&=( (*((unsigned short*)(pck+n-2))==0xD0CC) || (*((unsigned short*)(pck+n-2))==0xF1EA) );

//printf(" goodSiphr %d\n", good);
//printf(" goodSiphr %x\n", *((unsigned short*)(pck+n-2)));
  // Length of packet and declared length of content match
  nData = getLSBshort(pck+14);//TODO PENG CHANGE BACK TO 10
//printf(" nData + 24 = %d\n", nData+24);
  good&=(n==nData+24);//Peng change back to 20

//printf(" goodSiphr %d\n", good);
  return good;
}

// Verify full sanity of slow speed TDRSS data packet
int goodSIPlr(unsigned char *pck,int n){
  int good=1;
  unsigned char nData;
  
  // Leading word
  good&=(*pck==0xC1 || *pck==0xC2);
  // Length of packet and declared length of content match
  nData = *(pck+2);
  //  printf("  nData = %d,  %x %x %x %x\n", nData,
  //	 *(pck),*(pck+1),*(pck+2),*(pck+3)) ;
  good&=(n==nData+3);

  return good;
}


// Generic wrapper info returner
void wrapper(DataType_t type,unsigned char *pck,int n,unsigned int *nBuf,unsigned int *crc){
  switch(type){
  case LOS:{
    unsigned int losBuf;
    unsigned short losCrc;
//    wrapperLOS(pck,n,&losBuf,&losCrc);
    wrapperSIPhr(pck,n,&losBuf,&losCrc);
    *nBuf=losBuf;
    *crc=losCrc;
    return;
  }
  case SIPHR:{
    unsigned int hrBuf;
    unsigned short hrCrc;
    wrapperSIPhr(pck,n,&hrBuf,&hrCrc);
    *nBuf=hrBuf;
    *crc=hrCrc;
    return;
  }
  case SIPLR:{
    unsigned char nSeq,src;
    wrapperSIPlr(pck,n,&nSeq,&src);
    *nBuf=nSeq;
    *crc=src;
    return;
  }
  default:
    *nBuf=0;
    *crc=0;
  }
  return;
}

// Extract wrapper info for LOS packet
//not in use in anita3.  Raw_los data use wrapperSiphr
void wrapperLOS(unsigned char *pck,int n,unsigned int *nBuf,unsigned short *crc){
  unsigned short nData;
  *nBuf=*((unsigned int*)(pck+12));
  nData=*((unsigned short*)(pck+16));
  *crc=*((unsigned short*)(pck+nData+18));
  return;
}

// Extract wrapper info for high speed TDRSS packet
void wrapperSIPhr(unsigned char *pck,int n,unsigned int *nBuf,unsigned short *crc){
  unsigned short nData;
  *nBuf=*((unsigned int*)(pck+6));
  //nData=*((unsigned short*)(pck+10));
  nData=*((unsigned short*)(pck+14));
  //*crc=*((unsigned short*)(pck+nData+12));
  *crc=*((unsigned short*)(pck+nData+16));
  return;
}

// Extract wrapper info for slow speed TDRSS packet
void wrapperSIPlr(unsigned char *pck,int n,unsigned char *nSeq,unsigned char *src){
  *nSeq=*(pck+1);
  *src=(*pck)&0xf;
  return;
}

// Strips LOS header from data packet and returns pointer to data structure
// NB: There can be many data structures in the packet, so calling program 
// should take care of stripping them individually
void *depackLOS(unsigned char *pck,int n,unsigned short *nData){
  void *pckStruct_p;

  *nData = getLSBshort(pck+16);
  if(n<*nData+26) return NULL; // Something's bad
  if((*((unsigned short*)(pck+10)))&0x0002) --(*nData); // Remove fill byte if present
  pckStruct_p=(void*)(pck+18);
  return pckStruct_p;
}

// Strips fast TDRSS header from data packet and returns pointer to data structure
void *depackSIPhr(unsigned char *pck,int n,unsigned short *nData){
  void *pckStruct_p;

  *nData = getLSBshort(pck+14);//TODO PENG CHANGE BACK TO 10
  if(n<*nData+24) return NULL; // Something's bad TO PENG CHANGE BACK TO 20
  if((*((unsigned short*)(pck+4)))&0x0002) --(*nData); // Remove fill byte if present
  pckStruct_p=(void*)(pck+16); //TODO PENG CHANGE BACK TO 12
  return pckStruct_p;
}

// Strips slow TDRSS header from data packet and returns pointer to data structure
void *depackSIPlr(unsigned char *pck,int n,unsigned short *nData){
  void *pckStruct_p;

  *nData = (unsigned short)(*(pck+2));
  if(n<*nData+3) return NULL; // Something's bad
  pckStruct_p=(void*)(pck+3);
  return pckStruct_p;
}


// Return packet type
int pckType(const void *pckStruct_p){
  PacketCode_t *pckCode_p=(PacketCode_t*)pckStruct_p;
  switch(*pckCode_p&0xffff){
  case PACKET_HD:              return TYPE_HD;
  case PACKET_WV:              return TYPE_WV;
  case PACKET_PEDSUB_WV:       return TYPE_PEDSUB_WV;
  case PACKET_ENC_WV_PEDSUB:   return TYPE_ENC_WV_PEDSUB;
  case PACKET_SURF:            return TYPE_SURF;
  case PACKET_PEDSUB_SURF:     return TYPE_PEDSUB_SURF;
  case PACKET_ENC_SURF:        return TYPE_ENC_SURF;
  case PACKET_ENC_SURF_PEDSUB: return TYPE_ENC_SURF_PEDSUB;
  case PACKET_LAB_PED: 	       return TYPE_LAB_PED;
  case PACKET_SURF_HK:         return TYPE_SURF_HK;
  case PACKET_TURF_RATE:       return TYPE_TURF_RATE;
  case PACKET_GPS_ADU5_PAT:    return TYPE_GPS_ADU5_PAT;
  case PACKET_GPS_ADU5_VTG:    return TYPE_GPS_ADU5_VTG;
  case PACKET_GPS_ADU5_SAT:    return TYPE_GPS_ADU5_SAT;
  case PACKET_GPS_G12_POS:     return TYPE_GPS_G12_POS;
  case PACKET_GPS_G12_SAT:     return TYPE_GPS_G12_SAT;
  case PACKET_HKD:             return TYPE_HKD;
  case PACKET_HKD_SS:	       return TYPE_HKD_SS;//peng
  case PACKET_CMD_ECHO:        return TYPE_CMD_ECHO;
  case PACKET_MONITOR:         return TYPE_MONITOR;
  case PACKET_WAKEUP_LOS:      return TYPE_WAKEUP_LOS;
  case PACKET_WAKEUP_HIGHRATE: return TYPE_WAKEUP_HIGHRATE;
  case PACKET_WAKEUP_COMM1:    return TYPE_WAKEUP_COMM1;
  case PACKET_WAKEUP_COMM2:    return TYPE_WAKEUP_COMM2;
  case PACKET_SLOW1:           return TYPE_SLOW1;
  case PACKET_SLOW2:           return TYPE_SLOW2;
  case PACKET_SLOW_FULL:       return TYPE_SLOW_FULL;  
  case PACKET_ZIPPED_PACKET:   return TYPE_ZIPPED_PACKET;
  case PACKET_ZIPPED_FILE:     return TYPE_ZIPPED_FILE;
  case PACKET_RUN_START:       return TYPE_RUN_START;
  case PACKET_OTHER_MONITOR:   return TYPE_OTHER_MONITOR;
  case PACKET_GPS_GGA: 	       return TYPE_GPS_GGA;
  case PACKET_AVG_SURF_HK: 	   return TYPE_AVG_SURF_HK;
  case PACKET_SUM_TURF_RATE: 	   return TYPE_SUM_TURF_RATE;
  case PACKET_GPU_AVE_POW_SPEC: 	   return TYPE_GPU_AVE_POW_SPEC;
  default:                     return N_PCKTYPE;
  }
}

// Return data structures length
size_t pckSize(const void *pckStruct_p){
  PacketCode_t *pckCode_p=(PacketCode_t*)pckStruct_p;
  switch(*pckCode_p&0xffff){
  case PACKET_HD:          return sizeof(AnitaEventHeader_t);
  case PACKET_WV:          return sizeof(RawWaveformPacket_t);
  case PACKET_PEDSUB_WV:   return sizeof(PedSubbedWaveformPacket_t);
  case PACKET_ENC_WV_PEDSUB: return sizeof(EncodedPedSubbedChannelPacketHeader_t); // Not true size, since that is variable
  case PACKET_SURF:        return sizeof(RawSurfPacket_t);
  case PACKET_PEDSUB_SURF: return sizeof(PedSubbedSurfPacket_t);
  case PACKET_ENC_SURF:    return sizeof(EncodedSurfPacketHeader_t);  // Not true size, since that is variable
  case PACKET_ENC_SURF_PEDSUB: return sizeof(EncodedPedSubbedSurfPacketHeader_t);  // Not true size, since that is variable
  case PACKET_LAB_PED:     return sizeof(FullLabChipPedStruct_t);
  case PACKET_SURF_HK:     return sizeof(FullSurfHkStruct_t);
  case PACKET_TURF_RATE:   return sizeof(TurfRateStruct_t);
  case PACKET_GPS_ADU5_PAT:return sizeof(GpsAdu5PatStruct_t);
  case PACKET_GPS_ADU5_VTG:return sizeof(GpsAdu5VtgStruct_t);
  case PACKET_GPS_ADU5_SAT:return sizeof(GpsAdu5SatStruct_t);
  case PACKET_GPS_G12_POS: return sizeof(GpsG12PosStruct_t);
  case PACKET_GPS_G12_SAT: return sizeof(GpsG12SatStruct_t);
  case PACKET_HKD:         return sizeof(HkDataStruct_t);
  case PACKET_HKD_SS:      return sizeof(SSHkDataStruct_t);
  case PACKET_CMD_ECHO:    return sizeof(CommandEcho_t);
  case PACKET_MONITOR:     return sizeof(MonitorStruct_t);
  case PACKET_WAKEUP_LOS:  return WAKEUP_LOS_BUFFER_SIZE;
  case PACKET_WAKEUP_HIGHRATE: return WAKEUP_TDRSS_BUFFER_SIZE;
  case PACKET_WAKEUP_COMM1:return WAKEUP_LOW_RATE_BUFFER_SIZE;
  case PACKET_WAKEUP_COMM2:return WAKEUP_LOW_RATE_BUFFER_SIZE;
  case PACKET_SLOW1:       return 0; // Not supported any more
  case PACKET_SLOW2:       return 0;  // Doesn't exist 
  case PACKET_SLOW_FULL:   return sizeof(SlowRateFull_t);
  case PACKET_ZIPPED_PACKET: return sizeof(ZippedPacket_t); // Not true size, since that is variable
  case PACKET_ZIPPED_FILE: return sizeof(ZippedFile_t); // Not true size, since that is variable
  case PACKET_RUN_START:   return sizeof(RunStart_t);
  case PACKET_OTHER_MONITOR: return sizeof(OtherMonitorStruct_t);
  case PACKET_GPS_GGA: 	   return sizeof(GpsGgaStruct_t);
  case PACKET_AVG_SURF_HK: 	   return sizeof(AveragedSurfHkStruct_t);
  case PACKET_SUM_TURF_RATE: 	   return sizeof(SummedTurfRateStruct_t);
  case PACKET_GPU_AVE_POW_SPEC: 	   return sizeof(GpuPhiSectorPowerSpectrumStruct_t);
  default:                 return 0;
  }
}

// Return packet creation time 
long pckTime(const void *pckStruct_p){
  PacketCode_t *pckCode_p=(PacketCode_t*)pckStruct_p;
  if(*pckCode_p==PACKET_LAB_PED)
    return getLSBint(pckStruct_p+sizeof(GenericHeader_t)+sizeof(unsigned int));
  else if(*pckCode_p!=PACKET_WV && *pckCode_p!=PACKET_PEDSUB_WV &&
	  *pckCode_p!=PACKET_ENC_WV_PEDSUB && 
	  *pckCode_p!=PACKET_SURF && *pckCode_p!=PACKET_PEDSUB_SURF && 
	  *pckCode_p!=PACKET_ENC_SURF && *pckCode_p!=PACKET_ENC_SURF_PEDSUB &&
	  *pckCode_p!=PACKET_WAKEUP_LOS && *pckCode_p!=PACKET_WAKEUP_HIGHRATE &&
	  *pckCode_p!=PACKET_WAKEUP_COMM1 && *pckCode_p!=PACKET_WAKEUP_COMM2 &&
	  *pckCode_p!=PACKET_ZIPPED_PACKET)
    return getLSBint(pckStruct_p+sizeof(GenericHeader_t));
  else
    return 0;
}

// Return packet microsecond creation time 
long pckSubTime(const void *pckStruct_p){
  PacketCode_t *pckCode_p=(PacketCode_t*)pckStruct_p;
  if(*pckCode_p!=PACKET_WV && *pckCode_p!=PACKET_PEDSUB_WV && 
     *pckCode_p!=PACKET_ENC_WV_PEDSUB && 
     *pckCode_p!=PACKET_SURF && *pckCode_p!=PACKET_PEDSUB_SURF && 
     *pckCode_p!=PACKET_ENC_SURF && *pckCode_p!=PACKET_ENC_SURF_PEDSUB &&
     *pckCode_p!=PACKET_WAKEUP_LOS && *pckCode_p!=PACKET_WAKEUP_HIGHRATE &&
     *pckCode_p!=PACKET_WAKEUP_COMM1 && *pckCode_p!=PACKET_WAKEUP_COMM2 &&
     *pckCode_p!=PACKET_MONITOR && *pckCode_p!=PACKET_CMD_ECHO &&
     *pckCode_p!=PACKET_GPS_ADU5_SAT && *pckCode_p!=PACKET_GPS_G12_SAT &&
     *pckCode_p!=PACKET_ZIPPED_PACKET && *pckCode_p!=PACKET_ZIPPED_FILE)
    return getLSBint(pckStruct_p+sizeof(GenericHeader_t)+sizeof(int));
  else
    return 0;
}

// Return packet event number
long pckEventNumber(const void *pckStruct_p){
  PacketCode_t *pckCode_p=(PacketCode_t*)pckStruct_p;
  switch(*pckCode_p){
  case PACKET_HD:
    return getLSBint(pckStruct_p+sizeof(GenericHeader_t)+3*sizeof(long));
  case PACKET_WV:
  case PACKET_PEDSUB_WV:
  case PACKET_ENC_WV_PEDSUB:
  case PACKET_SURF:
  case PACKET_PEDSUB_SURF:
  case PACKET_ENC_SURF:
  case PACKET_ENC_SURF_PEDSUB:
    return getLSBint(pckStruct_p+sizeof(GenericHeader_t));
  default:
    return 0;
  }
}
    
long pckHKAnalogueCode(const void *pckStruct_p){
  PacketCode_t *pckCode_p=(PacketCode_t*)pckStruct_p;
  if(*pckCode_p==PACKET_HKD)
    return getLSBint(pckStruct_p+sizeof(GenericHeader_t)+2*sizeof(int));
  else
    return 0;
}

// Return packet directory base
const char *pckDirbase(int type){
  switch(type){
  case TYPE_HD:             return "event/";
  case TYPE_WV: 
  case TYPE_SURF: 
  case TYPE_ENC_SURF:       return "event/";  // These are raw waveforms
  case TYPE_PEDSUB_WV: 
  case TYPE_ENC_WV_PEDSUB:
  case TYPE_PEDSUB_SURF: 
  case TYPE_ENC_SURF_PEDSUB:return "event/";  // These are pedsubbed waveforms
  case TYPE_LAB_PED:        return "pedestal/";
  case TYPE_SURF_HK:        return "house/surfhk/";
  case TYPE_TURF_RATE:      return "house/turfhk/";
  case TYPE_GPS_ADU5_PAT:   return "house/gps/adu5/pat/";
  case TYPE_GPS_ADU5_VTG:   return "house/gps/adu5/vtg/";
  case TYPE_GPS_ADU5_SAT:   return "house/gps/adu5/sat/";
  case TYPE_GPS_G12_POS:    return "house/gps/g12/pos/";
  case TYPE_GPS_G12_SAT:    return "house/gps/g12/sat/";
  case TYPE_HKD:            return "house/hk/";
  case TYPE_HKD_SS:         return "house/hk/";//TODO peng
  case TYPE_CMD_ECHO:       return "house/cmd/";
  case TYPE_MONITOR:        return "house/monitor/";
  case TYPE_SLOW_FULL:      return "slow/";
  case TYPE_ZIPPED_FILE:    return "config/";
  case TYPE_RUN_START:      return "event/run/";
  case TYPE_OTHER_MONITOR:  return "house/monitor/other/";
  default:                  return NULL;
  }
}

// Return packet file base
const char *pckFilebase(int type){
  switch(type){
  case TYPE_HD:             return "hd";
  case TYPE_WV: 
  case TYPE_SURF: 
  case TYPE_ENC_SURF:       return "wv";  // These are raw waveforms
  case TYPE_PEDSUB_WV: 
  case TYPE_ENC_WV_PEDSUB:
  case TYPE_PEDSUB_SURF: 
  case TYPE_ENC_SURF_PEDSUB:return "pswv";  // These are pedsubbed waveforms
  case TYPE_LAB_PED:        return "peds";
  case TYPE_SURF_HK:        return "surfhk";
  case TYPE_TURF_RATE:      return "turfhk";
  case TYPE_GPS_ADU5_PAT:   return "pat";
  case TYPE_GPS_ADU5_VTG:   return "vtg";
  case TYPE_GPS_ADU5_SAT:   return "sat_adu5";
  case TYPE_GPS_G12_POS:    return "pos";
  case TYPE_GPS_G12_SAT:    return "sat";
  case TYPE_HKD:            return "hk_raw";
  case TYPE_HKD_SS:         return "hk_raw";//TODO peng
  case TYPE_CMD_ECHO:       return "cmd";
  case TYPE_MONITOR:        return "mon";
  case TYPE_SLOW_FULL:      return "slow";
  case TYPE_ZIPPED_FILE:    // Config filename handled differently
  case TYPE_RUN_START:      return "run";
  case TYPE_OTHER_MONITOR:  return "other";
  default:                  return NULL;
  }
}

char *pckFilename(const void *pckStruct_p){
  PacketCode_t *pckCode_p=(PacketCode_t*)pckStruct_p;
  if(*pckCode_p==PACKET_ZIPPED_FILE){
    ZippedFile_t *zip_p=(ZippedFile_t*)pckStruct_p;
    return zip_p->filename;
  }else
    return NULL;
}


// Return GPS location location
void pckLoc(const void *pckStruct_p,float *lat,float *lon,float *alt,long *tod){
  GenericHeader_t *gHdr=(GenericHeader_t*)pckStruct_p;
  if(gHdr->code==PACKET_GPS_ADU5_PAT){
    GpsAdu5PatStruct_t *gps_pat_p=(GpsAdu5PatStruct_t*)pckStruct_p;
    *lat=gps_pat_p->latitude;
    *lon=gps_pat_p->longitude;
    *alt=gps_pat_p->altitude;
    *tod=gps_pat_p->timeOfDay;
  }else if(gHdr->code==PACKET_GPS_G12_POS){
    GpsG12PosStruct_t *gps_pos_p=(GpsG12PosStruct_t*)pckStruct_p;
    *lat=gps_pos_p->latitude;
    *lon=gps_pos_p->longitude;
    *alt=gps_pos_p->altitude;
    *tod=gps_pos_p->timeOfDay;
  }
  // This is SLAC hangtest hack
  //  if(*lat==0 || *lon==0 || *alt==0){
  //    *lat=37.41711;
  //   *lon=-122.20116;
  //  *alt=39; // ????
  // }
  // End of SLAC hangtest hack
  return;
}

// Return payload orientation
void pckOrient(const void *pckStruct_p,float *heading,float *pitch,float *roll){
  GpsAdu5PatStruct_t *gps_pat_p=(GpsAdu5PatStruct_t*)pckStruct_p;
  if(gps_pat_p->gHdr.code==PACKET_GPS_ADU5_PAT){
    *heading=gps_pat_p->heading;
    *pitch=gps_pat_p->pitch;
    *roll=gps_pat_p->roll;
  }
  return;
}

// Return payload command
void pckCmdEcho(const void *pckStruct_p,short *good,unsigned char *cmdStr){
  CommandEcho_t *cmd_p=(CommandEcho_t*)pckStruct_p;
  if(cmd_p->gHdr.code==PACKET_CMD_ECHO){
    int i,n;
    *good=cmd_p->goodFlag;
    if(cmd_p->numCmdBytes>0){
      n=sprintf(cmdStr,"0x%x ",cmd_p->cmd[0]);
      for(i=1;i<cmd_p->numCmdBytes;++i)
	n+=sprintf(&cmdStr[n],"%d ",cmd_p->cmd[i]);
    }else
      cmdStr[0]='\0';
  }
  return;
}

// Return payload critical housekeeping
int pckCritical(const void *pckStruct_p,float *toti,float *pvv,float *tsbs,float *tplate){
  // Use tplate as it12 (bd3, ch 26)
  unsigned static short cal[3]={4055,4055,4055};
  unsigned static short avz[3]={2048,2048,2048};
  HkDataStruct_t *hk_p=(HkDataStruct_t*)pckStruct_p;
  if(hk_p->gHdr.code==PACKET_HKD){
    if(hk_p->ip320.code==IP320_CAL){ 
      cal[0]=hk_p->ip320.board[1].data[15];
      cal[1]=hk_p->ip320.board[1].data[37];
      cal[2]=hk_p->ip320.board[2].data[25];
      return 0;
    }else if(hk_p->ip320.code==IP320_AVZ){
      avz[0]=hk_p->ip320.board[1].data[15];
      avz[1]=hk_p->ip320.board[1].data[37];
      avz[2]=hk_p->ip320.board[2].data[25];
      return 0;
    }else{
      unsigned short raw[3];
      float calib[3];
      int i;
      raw[0]=hk_p->ip320.board[1].data[15];
      raw[1]=hk_p->ip320.board[1].data[37];
      raw[2]=hk_p->ip320.board[2].data[25];
      // Calibrate and return values
      for(i=0;i<3;++i){
	float slope=4.9 / (cal[i]-avz[i]);
	calib[i]=slope*(raw[i]-avz[i]);
      }
      *toti=calib[0]*20;
      *pvv=calib[1]*18.252;
      *tplate=calib[2]*100-273.15;
      // Average CPU temperature
      *tsbs=(hk_p->sbs.temp[0]+hk_p->sbs.temp[1])/2.;
      *tsbs/=40.;  // see anitaStructure.h for CR11 temp conversion. 
      return 1;
    }
  }
  return 0;
}
  
