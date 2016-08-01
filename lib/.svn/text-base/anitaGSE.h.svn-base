/*  Copyright (C) 2005 Predrag Miocinovic <predrag@phys.hawaii.edu> 

  This file is free software; as a special exception the author gives
  unlimited permission to copy and/or distribute it, with or without 
  modifications, as long as this notice is preserved. 

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY, to the extent permitted by law; without even the 
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Declarations for GSE software

  $Header$
*/

#ifndef ANITAGSE_H
#define ANITAGSE_H

#include "crc_simple.h"

#define SIPHR_MASK  0xf00d     // Leading 16-bit word of SIP high rate data
#define LOS_MASK    0xf00d     // Leading 16-bit word of LOS data
#define N_PCKTYPE 30

typedef enum{LOS,SIPHR,SIPLR} DataType_t;

// Simple function definitions
inline unsigned short getLSBshort(const unsigned char *ptr){
  return *((unsigned short*)ptr);
}

inline unsigned int getLSBint(const unsigned char *ptr){
  return *((unsigned int*)ptr);
}

inline unsigned long getLSBlong(const unsigned char *ptr){
  return *((unsigned long*)ptr);
}

inline unsigned short getMSBshort(const unsigned char *ptr){
  return (*ptr)*256+*(ptr+1);
}

inline unsigned long getMSBlong(const unsigned char *ptr){
  return (*ptr)*256*256*256+*(ptr+1)*256*256+*(ptr+2)*256+*(ptr+3);
}

// Function declarations
void *depackLOS(unsigned char *pck,int n,unsigned short *nData);
void *depackSIPhr(unsigned char *pck,int n,unsigned short *nData);
void *depackSIPlr(unsigned char *pck,int n,unsigned short*nData);

int pckType(const void *pckStruct_p);
size_t pckSize(const void *pckStruct_p);
long pckTime(const void *pckStruct_p);
long pckSubTime(const void *pckStruct_p);
long pckEventNumber(const void *pckStruct_p);
long pckHKAnalogueCode(const void *pckStruct_p);
char *pckFilename(const void *pckStruct_p);
void pckLoc(const void *pckStruct_p,float *lat,float *lon,float *alt,long *tod);
void pckOrient(const void *pckStruct_p,float *heading,float *pitch,float *roll);
int pckCritical(const void *pckStruct_p,float *toti,float *pvv,float *tsbs,float *tplate);
void pckCmdEcho(const void *pckStruct_p,short *good,unsigned char *cmdStr);
const char *pckDirbase(int type);
const char *pckFilebase(int type);

int goodLOS(unsigned char *pck,int n);
int goodSIPhr(unsigned char *pck,int n);
int goodSIPlr(unsigned char *pck,int n);

void wrapper(DataType_t type,unsigned char *pck,int n,unsigned int *nBuf,unsigned int *crc);
void wrapperLOS(unsigned char *pck,int n,unsigned int *nBuf,unsigned short *crc);
void wrapperSIPhr(unsigned char *pck,int n,unsigned int *nBuf,unsigned short *crc);
void wrapperSIPlr(unsigned char *pck,int n,unsigned char *nSeq,unsigned char *src);

// Translation between Ryan's packet codes and local sequential types
enum {
  TYPE_HD=0,
  TYPE_WV,
  TYPE_PEDSUB_WV,
  TYPE_ENC_WV_PEDSUB,
  TYPE_SURF,
  TYPE_PEDSUB_SURF,
  TYPE_ENC_SURF,
  TYPE_ENC_SURF_PEDSUB,
  TYPE_LAB_PED,
  TYPE_SURF_HK,
  TYPE_TURF_RATE,
  TYPE_GPS_ADU5_PAT,
  TYPE_GPS_ADU5_VTG,
  TYPE_GPS_ADU5_SAT,
  TYPE_GPS_G12_POS,
  TYPE_GPS_G12_SAT,
  TYPE_HKD,
  TYPE_HKD_SS,//peng
  TYPE_CMD_ECHO,
  TYPE_MONITOR,
  TYPE_WAKEUP_LOS,
  TYPE_WAKEUP_HIGHRATE,
  TYPE_WAKEUP_COMM1,
  TYPE_WAKEUP_COMM2,
  TYPE_SLOW1,
  TYPE_SLOW2,
  TYPE_SLOW_FULL,
  TYPE_ZIPPED_PACKET,
  TYPE_ZIPPED_FILE,
  TYPE_RUN_START,
  TYPE_OTHER_MONITOR,
 //ddm
  TYPE_GPS_GGA,
  TYPE_GPU_AVE_POW_SPEC,
  TYPE_SUM_TURF_RATE,
  TYPE_AVG_SURF_HK
};

#endif // ANITAGSE_H
