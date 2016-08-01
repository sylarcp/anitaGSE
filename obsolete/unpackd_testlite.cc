/* Daemon unpacking ANITA data. Based on unpackd.py prototype from anita-lite gse. 
   Written by: Predrag Miocinovic 06072005

 $Header: /cvsroot/anitacode/gse/unpack/unpackd.cc,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $
*/

#include <sstream>
#include <iostream>
#include <iomanip>
#include <libpq-fe.h>

#include "unpackd_testlite.h"
#include "AcqDAQ.h"

inline unsigned short getLSBshort(unsigned char *ptr){
  return *((unsigned short*)ptr);
}

inline unsigned long getLSBlong(unsigned char *ptr){
  return *((unsigned long*)ptr);
}

inline unsigned short getMSBshort(unsigned char *ptr){
  return (*ptr)*256+*(ptr+1);
}

inline unsigned long getMSBlong(unsigned char *ptr){
  return (*ptr)*256*256*256+*(ptr+1)*256*256+*(ptr+2)*256+*(ptr+3);
}


// CRC algorithm for two byte words
unsigned short crc_short(unsigned char* pck,int n){
  int i;
  unsigned short val,sum = 0;

  for(i=0;i<n;i+=2){
    val=*((unsigned short*)(pck+i));
    sum+=val;
  }
  return ((0xffff-sum)+1);
}

// Packet constructor, strips data common to all 
Packet::Packet(unsigned char *data,int n){
  if(n<1) throw Error::EmptyLine();  // Needs to have data
  if(n<10) throw Error::MalformatedPacket(data,n); // Length of wrapper only
  // Check CRC
  unsigned short crcOld=getLSBshort(data+n-2);
  unsigned short crcNew=crc_short(data,n-2);
  if(crcNew!=crcOld) throw Error::CRCError(data,n);
  fields.push_back(Word(crcOld,"crc"));
  
  // Strip wrapper bytes
  unsigned short evType=getLSBshort(data);
  if(evType!=0xBEFE) throw Error::NotAnitaPacket(data,n);
  unsigned long evNum=getLSBlong(data+2);
  unsigned long PCtick=getLSBlong(data+6);
  fields.push_back(Word(evType,"pcktype"));
  fields.push_back(Word(evNum,"pcknum"));
  fields.push_back(Word(PCtick,"pctick"));
		   
  return;
}
  
/* Stores packet into database */
bool Packet::store(PGconn *DBconn){
  // Create a string for SQL command
  ostringstream sqlcmd;
  sqlcmd << setprecision(6);

  // Prepare SQL command
  sqlcmd << "INSERT INTO " << table << " (";
  // Loop over all fields and record their names
  for(int i=0;i<fields.size();++i){
    sqlcmd << "\"" << fields[i].getTag() << "\"";
    if(i<fields.size()-1) sqlcmd << ",";
  }
  sqlcmd << ") VALUES (";
  // Loop over all fields and record their values
  for(int i=0;i<fields.size();++i){
    sqlcmd << fields[i];
    if(i<fields.size()-1) sqlcmd << ",";
  }
  sqlcmd << ");";

  //cerr << str << endl;

  // Execute SQL packet insertion
  PGresult *res=PQexec(DBconn,sqlcmd.str().c_str());
  // Verify result, very simplistic for now
  int code=PQresultStatus(res);
  PQclear(res);
  switch(code){
  case PGRES_COMMAND_OK:
    return 1;
  default:
    cerr << "PQ Error code: " << code << endl;
    cerr << sqlcmd.str().c_str() << endl;
    return 0;
  }
}

//Houskeeping1 constructor
Hk1Packet::Hk1Packet(unsigned char *data,int n)
  :Packet(data,n){
  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Set DB table name
  table="hk1";
  // Parse packet, first check length and type 
  unsigned char len=*pck;
  if(len!=29) throw Error::MalformatedPacket(data,n);
  unsigned char type=*(pck+1);
  if(type!=0x10) throw Error::FailedInitialization(data,n);
  
  //Get time
  unsigned long time=getLSBlong(pck+2);
  unsigned short timems=getLSBshort(pck+6);
  
  //Get DACs
  unsigned short dac[5];
  for(int i=0;i<5;++i) dac[i]=getMSBshort(pck+8+i*2);

  //Get ADCs
  unsigned short adc[5];
  for(int i=0;i<5;++i) adc[i]=getMSBshort(pck+18+i*2);

  //Get LINT bits
  unsigned char lint=pck[28];

  // Store values (not smart way at the moment)
  fields.reserve(17);
  fields.push_back(Word(time,"time"));
  fields.push_back(Word(timems,"ms"));
  fields.push_back(Word(dac[0],"dac0"));
  fields.push_back(Word(dac[1],"dac1"));
  fields.push_back(Word(dac[2],"dac2"));
  fields.push_back(Word(dac[3],"dac3"));
  fields.push_back(Word(dac[4],"dac4"));
  fields.push_back(Word(adc[0],"adc0"));
  fields.push_back(Word(adc[1],"adc1"));
  fields.push_back(Word(adc[2],"adc2"));
  fields.push_back(Word(adc[3],"adc3"));
  fields.push_back(Word(adc[4],"adc4"));
  fields.push_back(Word(lint,"lint"));

  return;
}

//Houskeeping2 constructor
Hk2Packet::Hk2Packet(unsigned char *data,int n)
  :Packet(data,n){
  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Set DB table name
  table="hk2";
  // Parse packet, first check length and type 
  unsigned char len=*pck;
  if(len!=77) throw Error::MalformatedPacket(data,n);
  unsigned char type=*(pck+1);
  if(type!=0x11) throw Error::FailedInitialization(data,n);
  
  //Get time
  unsigned long time=getLSBlong(pck+2);
  unsigned short timems=getLSBshort(pck+6);
  
  //Get sunsensor data
  unsigned short ssX[4];
  unsigned short ssY[4];
  unsigned short ssI[4];
  unsigned short sstemp[4];
  for(int i=0;i<4;++i){
    ssX[i]=getMSBshort(pck+8+i*6);
    ssY[i]=getMSBshort(pck+10+i*6);
    ssI[i]=getMSBshort(pck+12+i*6);
    sstemp[i]=getMSBshort(pck+32+i*2);
  }
  //Get temperatures
  unsigned short temp[2];
  for(int i=0;i<2;++i) temp[i]=getMSBshort(pck+40+i*2);
  //Get pressures
  unsigned short pres[2];
   for(int i=0;i<2;++i) temp[i]=getMSBshort(pck+56+i*2);

  //Get ADCs
  unsigned short adc[5];
  for(int i=0;i<5;++i) adc[i]=getMSBshort(pck+18+i*2);

  // Store values (not smart way at the moment)
  fields.reserve(24);
  fields.push_back(Word(time,"time"));
  fields.push_back(Word(timems,"ms"));
  fields.push_back(Word(ssX[0],"ssx0"));
  fields.push_back(Word(ssX[1],"ssx1"));
  fields.push_back(Word(ssX[2],"ssx2"));
  fields.push_back(Word(ssX[3],"ssx3"));
  fields.push_back(Word(ssY[0],"ssy0"));
  fields.push_back(Word(ssY[1],"ssy1"));
  fields.push_back(Word(ssY[2],"ssy2"));
  fields.push_back(Word(ssY[3],"ssy3"));
  fields.push_back(Word(ssI[0],"ssi0"));
  fields.push_back(Word(ssI[1],"ssi1"));
  fields.push_back(Word(ssI[2],"ssi2"));
  fields.push_back(Word(ssI[3],"ssi3"));
  fields.push_back(Word(sstemp[0],"sst0"));
  fields.push_back(Word(sstemp[1],"sst1"));
  fields.push_back(Word(sstemp[2],"sst2"));
  fields.push_back(Word(sstemp[3],"sst3"));
  fields.push_back(Word(temp[0],"temp0"));
  fields.push_back(Word(temp[1],"temp1"));
  fields.push_back(Word(pres[0],"pres0"));
  fields.push_back(Word(pres[1],"pres1"));
  return;
}

//Houskeeping4 constructor
Hk4Packet::Hk4Packet(unsigned char *data,int n)
  :Packet(data,n){
  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Set DB table name
  table="hk4";
  // Parse packet, first check length and type 
  unsigned char len=*pck;
  if(len!=83) throw Error::MalformatedPacket(data,n);
  unsigned char type=*(pck+1);
  if(type!=0x13) throw Error::FailedInitialization(data,n);
  
  //Get time
  unsigned long time=getMSBlong(pck+2);
  
  //Get packet queues info
  unsigned short pckwait[16];
  unsigned char pckage[16];
  for(int i=0;i<16;++i){
    pckwait[i]=getMSBshort(pck+6+i*2);
    pckage[i]=*(pck+38+i);
  }

  //Get event queues info
  unsigned short evwait[10];
  unsigned char evage[10];
  for(int i=0;i<10;++i){
    evwait[i]=getMSBshort(pck+54+i*2);
    evage[i]=*(pck+74+i);
  }

  // Store values (not smart way at the moment)
  fields.reserve(56);
  fields.push_back(Word(time,"time"));
  fields.push_back(Word(pckwait[0],"pckw0"));
  fields.push_back(Word(pckwait[1],"pckw1"));
  fields.push_back(Word(pckwait[2],"pckw2"));
  fields.push_back(Word(pckwait[3],"pckw3"));
  fields.push_back(Word(pckwait[4],"pckw4"));
  fields.push_back(Word(pckwait[5],"pckw5"));
  fields.push_back(Word(pckwait[6],"pckw6"));
  fields.push_back(Word(pckwait[7],"pckw7"));
  fields.push_back(Word(pckwait[8],"pckw8"));
  fields.push_back(Word(pckwait[9],"pckw9"));
  fields.push_back(Word(pckwait[10],"pckwa"));
  fields.push_back(Word(pckwait[11],"pckwb"));
  fields.push_back(Word(pckwait[12],"pckwc"));
  fields.push_back(Word(pckwait[13],"pckwd"));
  fields.push_back(Word(pckwait[14],"pckwe"));
  fields.push_back(Word(pckwait[15],"pckwf"));
  fields.push_back(Word(pckage[0],"pcka0"));
  fields.push_back(Word(pckage[1],"pcka1"));
  fields.push_back(Word(pckage[2],"pcka2"));
  fields.push_back(Word(pckage[3],"pcka3"));
  fields.push_back(Word(pckage[4],"pcka4"));
  fields.push_back(Word(pckage[5],"pcka5"));
  fields.push_back(Word(pckage[6],"pcka6"));
  fields.push_back(Word(pckage[7],"pcka7"));
  fields.push_back(Word(pckage[8],"pcka8"));
  fields.push_back(Word(pckage[9],"pcka9"));
  fields.push_back(Word(pckage[10],"pckaa"));
  fields.push_back(Word(pckage[11],"pckab"));
  fields.push_back(Word(pckage[12],"pckac"));
  fields.push_back(Word(pckage[13],"pckad"));
  fields.push_back(Word(pckage[14],"pckae"));
  fields.push_back(Word(pckage[15],"pckaf"));
  fields.push_back(Word(evwait[0],"evw0"));
  fields.push_back(Word(evwait[1],"evw1"));
  fields.push_back(Word(evwait[2],"evw2"));
  fields.push_back(Word(evwait[3],"evw3"));
  fields.push_back(Word(evwait[4],"evw4"));
  fields.push_back(Word(evwait[5],"evw5"));
  fields.push_back(Word(evwait[6],"evw6"));
  fields.push_back(Word(evwait[7],"evw7"));
  fields.push_back(Word(evwait[8],"evw8"));
  fields.push_back(Word(evwait[9],"evw9"));
  fields.push_back(Word(evage[0],"eva0"));
  fields.push_back(Word(evage[1],"eva1"));
  fields.push_back(Word(evage[2],"eva2"));
  fields.push_back(Word(evage[3],"eva3"));
  fields.push_back(Word(evage[4],"eva4"));
  fields.push_back(Word(evage[5],"eva5"));
  fields.push_back(Word(evage[6],"eva6"));
  fields.push_back(Word(evage[7],"eva7"));
  fields.push_back(Word(evage[8],"eva8"));
  fields.push_back(Word(evage[9],"eva9"));
  return;
}

//Command echo constructor
CmdPacket::CmdPacket(unsigned char *data,int n)
  :Packet(data,n){
  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Set DB table name
  table="cmd";
  // Parse packet, first check length and type 
  unsigned char len=*pck;
  if(len!=15) throw Error::MalformatedPacket(data,n);
  unsigned char type=*(pck+1);
  if(type!=0x00) throw Error::FailedInitialization(data,n);
  
  //Get time
  unsigned long time=getMSBlong(pck+2);
  unsigned char cs=*(pck+6);
  
  //Get command echo and response
  char cmd[9];
  sprintf(cmd,"%02x%02x%02x%02x",*(pck+7),*(pck+8),*(pck+9),*(pck+10));
  unsigned long ret=getMSBlong(pck+11);
  
  // Store values
  fields.reserve(8);
  fields.push_back(Word(time,"time"));
  fields.push_back(Word(cs,"cs"));
  fields.push_back(Word(cmd,"cmd"));
  fields.push_back(Word(ret,"ret"));
  return;
}

// Header packet constructor
HdPacket::HdPacket(unsigned char *data,int n)
  :Packet(data,n){
  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Set DB table name
  table="hd";
  // Parse packet, first check length and type 
  unsigned char len=*pck;
  if(len!=101) throw Error::MalformatedPacket(data,n);
  unsigned char type=*(pck+1);
  if((type>>4)!=0x05) throw Error::FailedInitialization(data,n);
  
  // Extract header 
  // Rely on the fact that structure packing is same on flight computer and here.
  // This is not(!) guaranteed, but since they are both gcc linux codes, eh....
  ACQHEADER *hdp=(ACQHEADER *)(pck+2);

  // Store fields
  fields.reserve(62);
  fields.push_back(Word(type&0xf,"trig"));
  fields.push_back(Word(hdp->evnumber,"evnum"));
  fields.push_back(Word(hdp->UTCsec,"time"));
  fields.push_back(Word(hdp->UTCms,"ms"));
  fields.push_back(Word(hdp->sampint,"sampint"));
  fields.push_back(Word(hdp->trgdel,"trgdel"));
  fields.push_back(Word(hdp->Nsamp,"nsamp"));
  fields.push_back(Word(hdp->Nseg,"nseg"));
  fields.push_back(Word(hdp->holdoff,"holdoff"));
  fields.push_back(Word(hdp->Dtype,"dtype"));
  fields.push_back(Word(hdp->trgcouple,"trgcouple"));
  fields.push_back(Word(hdp->trgslope,"trgslope"));
  fields.push_back(Word(hdp->trgsource,"trgsource"));
  fields.push_back(Word(hdp->trglevel,"trglevel"));
  fields.push_back(Word(hdp->clocktype,"clocktype"));
  fields.push_back(Word(hdp->ch_fs[0],"chfs0"));
  fields.push_back(Word(hdp->ch_fs[1],"chfs1"));
  fields.push_back(Word(hdp->ch_fs[2],"chfs2"));
  fields.push_back(Word(hdp->ch_fs[3],"chfs3"));
  fields.push_back(Word(hdp->ch_couple[0],"chcouple0"));
  fields.push_back(Word(hdp->ch_couple[1],"chcouple1"));
  fields.push_back(Word(hdp->ch_couple[2],"chcouple2"));
  fields.push_back(Word(hdp->ch_couple[3],"chcouple3"));
  fields.push_back(Word(hdp->ch_bw[0],"chbw0"));
  fields.push_back(Word(hdp->ch_bw[1],"chbw1"));
  fields.push_back(Word(hdp->ch_bw[2],"chbw2"));
  fields.push_back(Word(hdp->ch_bw[3],"chbw3"));
  fields.push_back(Word(hdp->Temp0,"temp0"));
  fields.push_back(Word(hdp->Temp1,"temp1"));
  fields.push_back(Word(hdp->ch_mean[0],"chmean0"));
  fields.push_back(Word(hdp->ch_mean[1],"chmean1"));
  fields.push_back(Word(hdp->ch_mean[2],"chmean2"));
  fields.push_back(Word(hdp->ch_mean[3],"chmean3"));
  fields.push_back(Word(hdp->ch_sdev[0],"chsdev0"));
  fields.push_back(Word(hdp->ch_sdev[1],"chsdev1"));
  fields.push_back(Word(hdp->ch_sdev[2],"chsdev2"));
  fields.push_back(Word(hdp->ch_sdev[3],"chsdev3"));
  fields.push_back(Word(hdp->GPSday,"gpsday"));
  fields.push_back(Word(hdp->GPShour,"gpshour"));
  fields.push_back(Word(hdp->GPSmin,"gpsmin"));
  fields.push_back(Word(hdp->GPSsec,"gpssec"));
  fields.push_back(Word(hdp->GPSfsec,"gpsfsec"));
  fields.push_back(Word(hdp->GPSlat,"gpslat"));
  fields.push_back(Word(hdp->GPSlong,"gpslong"));
  fields.push_back(Word(hdp->GPSalt,"gpsalt"));
  fields.push_back(Word(hdp->meanpwr[0],"meanpwr0"));
  fields.push_back(Word(hdp->meanpwr[1],"meanpwr1"));
  fields.push_back(Word(hdp->meanpwr[2],"meanpwr2"));
  fields.push_back(Word(hdp->meanpwr[3],"meanpwr3"));
  fields.push_back(Word(hdp->peakpwr[0],"peakpwr0"));
  fields.push_back(Word(hdp->peakpwr[1],"peakpwr1"));
  fields.push_back(Word(hdp->peakpwr[2],"peakpwr2"));
  fields.push_back(Word(hdp->peakpwr[3],"peakpwr3"));
  fields.push_back(Word(hdp->peakf[0],"peakf0"));
  fields.push_back(Word(hdp->peakf[1],"peakf1"));
  fields.push_back(Word(hdp->peakf[2],"peakf2"));
  fields.push_back(Word(hdp->peakf[3],"peakf3"));
  fields.push_back(Word(hdp->Priority,"priority"));

  return;
}

// Waveform packet constructor
WvPacket::WvPacket(unsigned char *data,int n)
  :Npck(42),Nsamp(1024),Packet(data,n){
  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Set DB table name
  table="wv";
  // Check packet type 
  unsigned char type=*(pck+1);

  if(fields.size()<5){ // This is first packet from this event
    // Get event info we need and store it
    int evnum=getLSBlong(pck+2);
    int time=getLSBlong(pck+6);
    int ms=getLSBshort(pck+10);
    
    fields.reserve(8);
    fields.push_back(Word(type&0xf,"trig"));
    fields.push_back(Word(evnum,"evnum"));
    fields.push_back(Word(time,"time"));
    fields.push_back(Word(ms,"ms"));
  }

  // Store info we need
  if((type>>4)==0x05){ // This is header packet
    insertHeader(data,n);
  }else if ((type>>4)==0x06){ // This is transient packet
    insertTransient(data,n);
  }else // Something is very wrong
    throw Error::FailedInitialization(data,n);

  return;
}
  
// Extract header info for waveform packet
void WvPacket::insertHeader(unsigned char *data,int n){
  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Get number of samples per waveform
  Nsamp=getLSBlong(pck+18);
  
  // Calculate number of transient packets to expect
  Npck=(Nsamp*Nch)/Nsamppck+1;  

  return;
}
  
// Extract transient info 
void WvPacket::insertTransient(unsigned char *data,int n,bool validated){
  if(!validated){ // Check packets CRC
    // Check CRC
    unsigned short crcOld=getLSBshort(data+n-2);
    unsigned short crcNew=crc_short(data,n-2);
    if(crcNew!=crcOld) throw Error::CRCError(data,n);
  }

  // Skip wrapper 
  unsigned char *pck=&data[10];
  // Get packet length
  unsigned char len=*pck;
  // Get transient sample number
  int N=getLSBshort(pck+12);
  
  // Calculate expected length 
  int explen=(N<Npck-1?Nsamppck:Nch*Nsamp-(Nsamppck*(Npck-1)))+13; // 13 header bytes
  if(explen!=len){
    fprintf(stderr,"evnum %d, Npck %d Nseg %d explen %d len %d\n",fields[5].getInt(),Npck,N,explen,len);
    throw Error::MalformatedPacket(data,n);
  }

  // Create array to hold packet data 
  unsigned char wv[98];
  memcpy(wv,pck+14,explen);

  // Insert wv data into hashmap
  pair<hash_map<int,unsigned char*>::iterator,bool> result=
      wvdata.insert(make_pair(N,wv));
  if(!result.second){;} // Insertion failed, e.g. due to duplicate entry, ignore it 

  return;
}
  
bool WvPacket::store(PGconn *DBconn){
  if(!Packet::store(DBconn)) return 0; // Store fields data
  // Express waveform data as one long array
  unsigned char wvfull[4096*4*4];
  for(int i=0;i<Npck;++i){
    hash_map<int,unsigned char*>::iterator wv_p=wvdata.find(i);
    int l=(i<Npck-1)?Nsamppck:(Nch*Nsamp-(Nsamppck*(Npck-1)));
    if(wv_p!=wvdata.end())
      memcpy(&wvfull[i*Npck],wv_p->second,l);
    else
      for(int n=0;n<l;++n) wvfull[i*Npck+n]=0;
  }

  // Assemble transient data into channel based arrays and store them
  ostringstream sqlcmd;
  sqlcmd << "UPDATE " << table << " SET ";
  for(int ch=0;ch<Nch;++ch){
    sqlcmd << "ch" << ch+1 << "='{";
    for(int i=0;i<Nsamp;++i){
      sqlcmd << int(wvfull[ch*Nsamp+i]);
      if(i<Nsamp-1) sqlcmd << ",";
    }
    sqlcmd << "}'";
    if(ch<Nch-1) sqlcmd << ",";
  } 
  sqlcmd << " WHERE (time=" << fields[6].getInt() << " AND ms=" << fields[7].getInt() <<");";

  // Execute SQL packet insertion
  PGresult *res=PQexec(DBconn,sqlcmd.str().c_str());
  // Verify result, very simplistic for now
  int code=PQresultStatus(res);
  PQclear(res);
  switch(code){
  case PGRES_COMMAND_OK:
    return 1;
  default:
    cerr << "PQ Error code: " << code << endl;
    cerr << sqlcmd.str().c_str() << endl;
    return 0;
  }
}
