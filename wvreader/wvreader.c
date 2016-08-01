/* Program that takes raw encoded waveform file and generates calibrated and
   rotated waveforms.

   Output format (repeats for each event in input file)
   event number - unsigned long
   chip number  - unsigned long
   waveforms    - 9*9*260 floats (surf, chan, sca ordering)

   Pedestals are expected in (Ryan's) raw format; 
   9*4*9*260 float (surf, chip, chan, sca)
   
   Usage: wvreader [-p <pedestal file>] -i <input file> -o <output file> [-n num events]

*/

#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <includes/anitaStructures.h>
#include <compressLib/compressLib.h>

#define ADCMV 1.17   /* mV/adc, per Gary's email of 05/04/2006 */
#define EVENTS_PER_FILE 100

int readEVfile(gzFile in,SurfChannelPedSubbed_t **wv){
  int i,j,evnum,goodStream=1;
  AnitaEventBody_t event;

  if(gzread(in,&event,sizeof(AnitaEventBody_t))!=sizeof(AnitaEventBody_t)){
    fprintf(stderr,"Event packet file is not of expected size\n");
    goodStream=0;
  }else{
    evnum=event.eventNumber;
    // Store data in local format
    for(i=0;i<ACTIVE_SURFS;++i)
      for(j=0;j<CHANNELS_PER_SURF;++j){
	memcpy(wv[i][j].data,event.channel[i*CHANNELS_PER_SURF+j].data,sizeof(unsigned short)*MAX_NUMBER_SAMPLES);
	wv[i][j].header=event.channel[i*CHANNELS_PER_SURF+j].header;
      }
  }
  return goodStream?evnum:-1;  
}

int readPSEVfile(gzFile in,SurfChannelPedSubbed_t **wv){
  int i,j,evnum,goodStream=1;
  PedSubbedEventBody_t event;
  if(gzread(in,&event,sizeof(PedSubbedEventBody_t))!=sizeof(PedSubbedEventBody_t)){
    fprintf(stderr,"Pedsubbed event packet file is not of expected size\n");
    goodStream=0;
  }else{
    evnum=event.eventNumber;
    // Store data in local format
    for(i=0;i<ACTIVE_SURFS;++i)
      for(j=0;j<CHANNELS_PER_SURF;++j){
	memcpy(wv[i][j].data,event.channel[i*CHANNELS_PER_SURF+j].data,sizeof(short)*MAX_NUMBER_SAMPLES);
	wv[i][j].header=event.channel[i*CHANNELS_PER_SURF+j].header;
      }
  }
  return goodStream?evnum:-1;  
}

int readENCEVfile(gzFile in,SurfChannelPedSubbed_t **wv){
  int i,j,evnum,goodStream=1;
  EncodedEventWrapper_t encEventWrapper;
  EncodedSurfChannelHeader_t encChanHeader;
  EncodedSurfPacketHeader_t encSurfPacket;

  // Read event wrapper
  if(gzread(in,&encEventWrapper,sizeof(EncodedEventWrapper_t))!=sizeof(EncodedEventWrapper_t)){
    fprintf(stderr,"Encoded event packet file is not of expected size\n");
    goodStream=0;
  }else
    evnum=encEventWrapper.eventNumber;

  // Loop over surfs
  for(i=0;goodStream && i<ACTIVE_SURFS;++i){
    if(gzread(in,&encSurfPacket,sizeof(EncodedSurfPacketHeader_t))!=
       sizeof(EncodedSurfPacketHeader_t)){
      fprintf(stderr,"Encoded event packet file is not of expected size\n");
      goodStream=0;
      break;
    }else{ // Read surf channels
      for(j=0;j<CHANNELS_PER_SURF;++j){
	if(gzread(in,&encChanHeader,sizeof(EncodedSurfChannelHeader_t))!=
	   sizeof(EncodedSurfChannelHeader_t)){
	  fprintf(stderr,"Encoded event packet file is not of expected size\n");
	  goodStream=0;
	  break;
	}else{  // Read waveform data
	  char encdata[MAX_NUMBER_SAMPLES*2]; // This is maximum expected size
	  if(gzread(in,encdata,encChanHeader.numBytes)!=encChanHeader.numBytes){
	    fprintf(stderr,"Encoded event packet file is not of expected size\n"),
	      goodStream=0;
	    break;
	  }else{ // Decode and store 
	    SurfChannelFull_t tmpwv;
	    CompressErrorCode_t retVal=decodeChannel(&encChanHeader,encdata,&tmpwv);
	    if(retVal!=COMPRESS_E_OK){
	      fprintf(stderr,"Decompression error encountered (%d)",retVal);
	      goodStream=0;
	    }else{
	      memcpy(wv[i][j].data,tmpwv.data,sizeof(short)*MAX_NUMBER_SAMPLES);
	      wv[i][j].header=tmpwv.header;
	    }
	  }
	  if(!goodStream) break;
	}
      }
    }
  }

  return goodStream?evnum:-1;
}

int readPSENCEVfile(gzFile in,SurfChannelPedSubbed_t **wv){
  int i,j,evnum,goodStream=1;
  EncodedEventWrapper_t encEventWrapper;
  EncodedSurfChannelHeader_t encChanHeader;
  EncodedPedSubbedSurfPacketHeader_t encSurfPacket;

  // Read event wrapper
  if(gzread(in,&encEventWrapper,sizeof(EncodedEventWrapper_t))!=sizeof(EncodedEventWrapper_t)){
    fprintf(stderr,"Pedsubbed encoded event packet file is not of expected size\n");
    goodStream=0;
  }else
    evnum=encEventWrapper.eventNumber;

  // Loop over surfs
  for(i=0;goodStream && i<ACTIVE_SURFS;++i){
    if(gzread(in,&encSurfPacket,sizeof(EncodedPedSubbedSurfPacketHeader_t))!=
       sizeof(EncodedPedSubbedSurfPacketHeader_t)){
      fprintf(stderr,"Pedsubbed encoded event packet file is not of expected size\n");
      goodStream=0;
      break;
    }else{ // Read surf channels
      for(j=0;j<CHANNELS_PER_SURF;++j){
	if(gzread(in,&encChanHeader,sizeof(EncodedSurfChannelHeader_t))!=
	   sizeof(EncodedSurfChannelHeader_t)){
	  fprintf(stderr,"Pedsubbed encoded event packet file is not of expected size\n");
	  goodStream=0;
	  break;
	}else{  // Read waveform data
	  char encdata[MAX_NUMBER_SAMPLES*2]; // This is maximum expected size
	  if(gzread(in,encdata,encChanHeader.numBytes)!=encChanHeader.numBytes){
	    fprintf(stderr,"Pedsubbed encoded event packet file is not of expected size\n"),
	      goodStream=0;
	    break;
	  }else{ // Decode and store 
	    CompressErrorCode_t retVal=decodePSChannel(&encChanHeader,encdata,&wv[i][j]);
	    if(retVal!=COMPRESS_E_OK){
	      fprintf(stderr,"Decompression error encountered (%d)",retVal);
	      goodStream=0;
	    }
	  }
	  if(!goodStream) break;
	}
      }
    }
  }

  return goodStream?evnum:-1;
}

void usage(const char *progname){
  fprintf(stderr,"Usage: %s [-p <flight pedestal file>] [-P <raw pedestal file>] -i <input file> -o <output file> [-n <events to process>; default %d]\n",progname,EVENTS_PER_FILE);
  fprintf(stderr,"Output format (repeats for each event in input file):\n"
	  "\tevent number - unsigned long\n"
	  "\tchip number  - unsigned long\n"
	  "\twaveforms    - 9*9*260 floats (surf, chan, sca ordering)\n"); 
  return;
}

int main(int argc, char *argv[]){
  float pedestalData[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES]; 
  PedestalStruct_t ped;  // For reading flight format pedestal files
  int pedIn=0;
  SurfChannelPedSubbed_t **wv;
  FILE *fp;
  gzFile in;
  int goodStream=1;
  int nProcessed=0;
  int nExpected=EVENTS_PER_FILE;
  int i,j,k,l,chip;
  int evnum;
  extern int optind;
  extern char *optarg;
  const char *pedFile=NULL;
  const char *rawPedFile=NULL;
  const char *inFile=NULL;
  const char *outFile=NULL;
  int c;
  int errflg=0;
  int (*readEvent)(gzFile, SurfChannelPedSubbed_t**) = NULL; 
  int needCal=0;

  while ((c = getopt(argc, argv, "p:P:i:o:n:")) != EOF) {
    switch(c){
    case 'p':
      pedFile=optarg;
      break;
    case 'P':
      rawPedFile=optarg;
      break;
    case 'i':
      inFile=optarg;
      break;
    case 'o':
      outFile=optarg;
      break;
    case 'n':
      nExpected=atoi(optarg);
      break;
    case ':':
    case '?':
    default:
      errflg++;
      break;
    }
  }

  if(errflg || !inFile || !outFile){
    usage(argv[0]);
    exit(-1);
  }


  // Load pedestals
  if(rawPedFile){
    if((fp=fopen(rawPedFile,"r"))==NULL){
      fprintf(stderr,"%s: Failed to open raw pedestal file %s.\n", argv[0],rawPedFile);
      exit(-1);
    }
    if(fread(pedestalData,sizeof(float),ACTIVE_SURFS*LABRADORS_PER_SURF*CHANNELS_PER_SURF*MAX_NUMBER_SAMPLES,fp)!=
       ACTIVE_SURFS*LABRADORS_PER_SURF*CHANNELS_PER_SURF*MAX_NUMBER_SAMPLES){
      fprintf(stderr,"%s: Error reading from raw pedestal file %s.\n", argv[0],
	      rawPedFile);
      exit(-1);
    }
    fclose(fp);
    pedIn=1;
  }

  if(pedFile){
    if((fp=fopen(pedFile,"r"))==NULL){
      fprintf(stderr,"%s: Failed to open pedestal file %s.\n", argv[0],pedFile);
      exit(-1);
    }
    if(fread(&ped,sizeof(PedestalStruct_t),1,fp)!=1){
      fprintf(stderr,"%s: Error reading from pedestal file %s.\n", argv[0],
	      pedFile);
      exit(-1);
    }
    fclose(fp);
    for(i=0;i<ACTIVE_SURFS;++i)
      for(j=0;j<LABRADORS_PER_SURF;++j)
	for(k=0;k<CHANNELS_PER_SURF;++k)
	  for(l=0;l<MAX_NUMBER_SAMPLES;++l)
	    pedestalData[i][j][k][l]=ped.thePeds[i][j][k][l]<<1; 
    pedIn=1;
  }
  
  /* There are 4 types of possible ways to store waveform data on flight computer.
     1. Raw (non-pedsubtracted) unencoded; using structure AnitaEventBody_t, file prefix 'ev'
     2. Pedestal subtracted, unencoded; using structure PedSubbedEventBody_t, file prefix 'psev'
     3. Raw (non-pedsubtracted) encoded; using variable lenght format, but headed by structure
        EncodedSurfPacketHeader_t, file prefix 'encev'
     4. Pedestal subtracted, encoded;  using variable lenght format, but headed by structure
        EncodedPedSubbedSurfPacketHeader_t, file prefix 'psencev'
  */
  if(!strncmp(basename(inFile),"ev",2)){
    readEvent=readEVfile;
    needCal=1;
  }else if(!strncmp(basename(inFile),"psev",4)){
    readEvent=readPSEVfile;
    needCal=0;
  }else if(!strncmp(basename(inFile),"encev",5)){
    readEvent=readENCEVfile;
    needCal=1;
  }else if(!strncmp(basename(inFile),"psencev",7)){
    readEvent=readPSENCEVfile;
    needCal=0;
  }else{
    fprintf(stderr,"Couldn't determine data format of input file %s.\n",inFile);
    exit(-1);
  }
   
  if(needCal && !pedIn){
    fprintf(stderr,"The input file requires calibration, but no pedestals specified.\n");
    exit(-1);
  }
	    
 
  // Open input file 
  if((in=gzopen(inFile,"rb"))==NULL){
    fprintf(stderr,"%s: Failed to open input file %s.\n", argv[0],inFile);
    exit(-1);
  }

  // Open output file 
  if((fp=fopen(outFile,"w"))==NULL){
    fprintf(stderr,"%s: Failed to open output file %s.\n", argv[0],outFile);
    exit(-1);
  }

  // Allocate memory we need
  wv=malloc(sizeof(SurfChannelPedSubbed_t*)*ACTIVE_SURFS);
  for(i=0;i<ACTIVE_SURFS;++i)
    wv[i]=malloc(sizeof(SurfChannelPedSubbed_t)*CHANNELS_PER_SURF);

  while(goodStream && nProcessed<nExpected){
    // Reset local waveform storage
    for(i=0;i<ACTIVE_SURFS;++i)
      for(j=0;j<CHANNELS_PER_SURF;++j)
	for(k=0;k<MAX_NUMBER_SAMPLES;++k)
	  wv[i][j].data[k]=0;

    // Read next event
    evnum=readEvent(in,wv);
    goodStream=(evnum>0);

    // Calibrate rotate and output
    if(goodStream){
      float calwv[MAX_NUMBER_SAMPLES];
      float v[MAX_NUMBER_SAMPLES];

      if(fwrite(&evnum,sizeof(unsigned long),1,fp)!=1){
	fprintf(stderr,"%s: Error writing evnum to output file\n",argv[0]);
	break;
      }
      chip=wv[0][0].header.chipIdFlag&0x3;
      if(fwrite(&chip,sizeof(int),1,fp)!=1){
	fprintf(stderr,"%s: Error writing chipId to output file\n",argv[0]);
	break;
      }

      for(i=0;i<ACTIVE_SURFS;++i)
	for(j=0;j<CHANNELS_PER_SURF;++j){
	  int nSurf,nChan,nChip,hbwrap,ir;
	  char hbextra;
	  short hbstart,hbend;

	  nSurf=wv[i][j].header.chanId/CHANNELS_PER_SURF;
	  nChan=wv[i][j].header.chanId%CHANNELS_PER_SURF;
	  nChip=wv[i][j].header.chipIdFlag&0x03;
	  if(nSurf<0 || nSurf>=ACTIVE_SURFS ||
	     nChan<0 || nChan>=CHANNELS_PER_SURF ||
	     nChip<0 || nChip>=LABRADORS_PER_SURF){
	    fprintf(stderr,"Bad waveform id; SURF %d CHAN %d CHIP %d @ (%d,%d)\n",
		    nSurf,nChan,nChip,i,j);
	    break;
	  }

	  hbwrap=wv[i][j].header.chipIdFlag&0x08;
	  hbextra=(wv[i][j].header.chipIdFlag&0xf0)>>4;
	  hbstart=wv[i][j].header.firstHitbus;
	  hbend=wv[i][j].header.lastHitbus+hbextra;
	  // Calibrate ...
	  for(k=0;k<MAX_NUMBER_SAMPLES;++k){
	    if(wv[i][j].data[k]==0){
	      calwv[k]=0;
	    }else{
	      if(needCal)
		calwv[k]=((wv[i][j].data[k]&0xfff)-pedestalData[nSurf][nChip][nChan][k])*ADCMV;
	      else
		calwv[k]=(wv[i][j].data[k]<<1)*ADCMV; // Data bitshifted with ped subtraction by flight CPU
	    }
	  }
	  // ... and rotate
	  ir=0;
	  if(hbwrap){ // Wrapped hitbus
	    for(k=hbstart+1;k<hbend;++k)
	      v[ir++]=calwv[k];
	  }else{
	    for(k=hbend+1;k<MAX_NUMBER_SAMPLES;++k) 
	      v[ir++]=calwv[k];	
	    for(k=1;k<hbstart;++k)  // Skip 0th SCA
	      v[ir++]=calwv[k];
	  }
	  // Fill in remaining bins with zeros
	  for(k=ir;k<MAX_NUMBER_SAMPLES;++k)
	    v[k]=0
;
	  // Output waveform
	  if(fwrite(v,sizeof(float),MAX_NUMBER_SAMPLES,fp)!=MAX_NUMBER_SAMPLES){
	    fprintf(stderr,"%s: Error writing waveform to output file\n",argv[0]);
	    break;
	  }
      
	}
      ++nProcessed;
    }
  }

  // Close files
  gzclose(in);
  fclose(fp);

  printf("%s: Processed %d events\n",argv[0],nProcessed);

  // Free memory
  for(i=0;i<ACTIVE_SURFS;++i) free(wv[i]);
  free(wv);

  exit(0);
}
  
