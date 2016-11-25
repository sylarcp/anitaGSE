/* Daemon unpacking ANITA data. Based on unpackd.py prototype from anita-lite gse. 
   Written by: Predrag Miocinovic 06072005

*/

#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

extern "C"{
#include <string.h>
#include <libpq-fe.h>
#include <zlib.h>
}
#include <compressLib/compressLib.h>
#include "unpackClasses.h"
//std::ofstream calib_out("calib_out.txt");
// Packet constructor, strips data common to all 
XferPacket::XferPacket(unsigned char *data,int n){
  DataType_t type;
  bool crcWrapper=Packet::checkWrapper(data,n,&type);
  
  // Loop over all data packets in the buffer and store them
  int nProcessed=0;
  while(nProcessed<nData){
    void *ptr=(void*)((unsigned char *)pckStruct_p+nProcessed);
   //  cerr<<"XferPacket::PacketCode_t = 0x"<<hex<<((*(unsigned short*)ptr))<<endl;//peng
   // void *ptr2=(void*)((unsigned char *)pckStruct_p+nProcessed);
      //cerr<<"XferPacket::AuxPacketCode_t = 0x"<<hex<<((*(unsigned int*)ptr2)&0xffff0000)<<"  "<<nProcessed<<endl;//peng
   // void *ptr3=(void*)((unsigned char *)pckStruct_p);
      //cerr<<"XferPacket::AuxPacketCode_t = 0x"<<hex<<((*(unsigned int*)ptr3)&0xffff0000)<<endl;//peng

      //cerr<<"XferPacket::PacketCode_t = 0x"<<hex<<((*(unsigned short*)ptr))<<endl;//peng
    int pcktype=pckType(ptr);
      //cerr<<"XferPacket::pcktype = "<<pcktype <<" type = "<<type<< endl;//peng
      //cerr<<"XferPacket::nProcessed = "<<nProcessed <<dec<<" nData " << nData << hex <<  endl;//peng
    int size=pckSize(ptr);
      //cerr<<"XferPacket::size = "<<size<<endl;

    if(!size){
      //cerr<<"XferPacket:packets.size() = "<< packets.size() <<  endl;//peng
      if(packets.size()){ // Don't throw if some data read in ok
	cerr << "Packet code error: " << hex << *((PacketCode_t*)ptr) << dec << " Full: " <<  nData << " Processed: " << nProcessed << " Size:" << size << endl;
	break; 
      }

      throw Error::UnmatchedPacketType(data,n,*((PacketCode_t*)ptr));
    }
    // Check that received and expected buffer size match; 
    // i.e. make sure there are no buffer overruns
    if(nProcessed+size>nData){
      cerr << "Buffer overrun! Received " << nData << " bytes, but internally declared " << (nProcessed+size)<< endl;
      cerr << "Processed: " << nProcessed << ", skipping last " << (nData-nProcessed) << " bytes, declared as packet code " << hex << *((PacketCode_t*)ptr) << dec << endl;
      break;
    }

    try{
      // Some packets need pre-processing
      switch(pcktype){
      case TYPE_ENC_WV_PEDSUB:{  // This is pedsubbed encoded waveform packet, so need to do some monkeying
	EncodedPedSubbedChannelPacketHeader_t *ewv_p=(EncodedPedSubbedChannelPacketHeader_t*)ptr;
	if((ewv_p->gHdr.code&0xffff)!=PACKET_ENC_WV_PEDSUB) throw Error::ParseError();
	// Run checksum for entire packet...
	bool crcPacket=InternalCRC((unsigned char*)ptr);
	int offset=sizeof(EncodedPedSubbedChannelPacketHeader_t);
	// Read encoded header and data
	EncodedSurfChannelHeader_t *chanHead=(EncodedSurfChannelHeader_t*)((char*)ptr+offset);
	try{
	  // Create pedsubbed waveform packet and fill it
	  PedSubbedWaveformPacket_t wv;
	  wv.gHdr.code=ewv_p->gHdr.code;
	  wv.gHdr.packetNumber=ewv_p->gHdr.packetNumber;	
	  wv.gHdr.numBytes=sizeof(PedSubbedWaveformPacket_t);
	  wv.gHdr.feByte=ewv_p->gHdr.feByte;
	  wv.gHdr.verId=ewv_p->gHdr.verId;
	  wv.gHdr.checksum=0; // No checksum
	  wv.eventNumber=ewv_p->eventNumber;
	  wv.whichPeds=ewv_p->whichPeds;
	  // Run checksum for encoded waveform only
	  int crcPacketIndividual=crcPacket+(((int)WaveformCRC((unsigned char*)ptr+offset))<<1);
	  // Decompress waveform
	  unsigned char *encdata=(unsigned char*)ptr+offset+sizeof(EncodedSurfChannelHeader_t);
	  CompressErrorCode_t retVal=decodePSChannel(chanHead,encdata,&wv.waveform);
	  if(retVal==COMPRESS_E_BAD_CODE)
	    throw Error::UnknownEncodingType((unsigned char*)chanHead,
					     sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes,
					     ewv_p->eventNumber,(int)chanHead->encType);
	  if(retVal!=COMPRESS_E_OK) 
	    throw Error::DecodingError((unsigned char*)chanHead,
				       sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes,
				       ewv_p->eventNumber,(int)retVal);
	  // Parse new packet
	  packets.push_back(Packet((void*)(&wv),wv.gHdr.numBytes,type,crcWrapper,crcPacketIndividual));
	}
	catch(Error::BaseError &e){
	  cerr << e.message << " " << e.action << endl;
	  if(e.info[0]) cerr << e.info << endl;
	}
	// Account for channel header and waveform in data stream
	size+=sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes;  
      break;
      }
      case TYPE_SURF:{  // This is raw surf packet so we need to do some monkeying
	RawSurfPacket_t *surf_p=(RawSurfPacket_t*)ptr;
	if((surf_p->gHdr.code&0xffff)!=PACKET_SURF) throw Error::ParseError();
	// check original checksum
	bool crcPacket=InternalCRC((unsigned char*)ptr);
	// Loop over all waveforms in surf packet and create packets for
	// individual waveforms
	packets.reserve(packets.size()+CHANNELS_PER_SURF);
	for(int i=0;i<CHANNELS_PER_SURF;++i){
	  RawWaveformPacket_t wv;
	  wv.gHdr.code=surf_p->gHdr.code;
	  wv.gHdr.packetNumber=surf_p->gHdr.packetNumber;	
	  wv.gHdr.numBytes=sizeof(RawWaveformPacket_t);
	  wv.gHdr.feByte=surf_p->gHdr.feByte;
	  wv.gHdr.verId=surf_p->gHdr.verId;
	  wv.gHdr.checksum=0; // No checksum
	  wv.eventNumber=surf_p->eventNumber;
	  wv.waveform=surf_p->waveform[i];
	  try{
	    packets.push_back(Packet((void*)(&wv),wv.gHdr.numBytes,type,crcWrapper,crcPacket));
	  }
	  catch(Error::BaseError &e){
	    cerr << e.message << " " << e.action << endl;
	    if(e.info[0]) cerr << e.info << endl;
	  }
	}
	break;
      }
      case TYPE_PEDSUB_SURF:{  // This is pedsubbed surf packet so we need to do some monkeying
	PedSubbedSurfPacket_t *surf_p=(PedSubbedSurfPacket_t*)ptr;
	if((surf_p->gHdr.code&0xffff)!=PACKET_PEDSUB_SURF) throw Error::ParseError();
	// check original checksum
	bool crcPacket=InternalCRC((unsigned char*)ptr);
	// Loop over all waveforms in surf packet and create packets for
	// individual waveforms
	packets.reserve(packets.size()+CHANNELS_PER_SURF);
	for(int i=0;i<CHANNELS_PER_SURF;++i){
	  PedSubbedWaveformPacket_t wv;
	  wv.gHdr.code=surf_p->gHdr.code;
	  wv.gHdr.packetNumber=surf_p->gHdr.packetNumber;	
	  wv.gHdr.numBytes=sizeof(PedSubbedWaveformPacket_t);
	  wv.gHdr.feByte=surf_p->gHdr.feByte;
	  wv.gHdr.verId=surf_p->gHdr.verId;
	  wv.gHdr.checksum=0; // No checksum
	  wv.eventNumber=surf_p->eventNumber;
	  wv.whichPeds=surf_p->whichPeds;
	  wv.waveform=surf_p->waveform[i];
	  try{
	    packets.push_back(Packet((void*)(&wv),wv.gHdr.numBytes,type,crcWrapper,crcPacket));
	  }
	  catch(Error::BaseError &e){
	    cerr << e.message << " " << e.action << endl;
	    if(e.info[0]) cerr << e.info << endl;
	  }
	}
	break;
      }
      case TYPE_ENC_SURF:{     // This is encoded surf packet so we need to do some monkeying
	EncodedSurfPacketHeader_t *esurf_p=(EncodedSurfPacketHeader_t*)ptr;
	if((esurf_p->gHdr.code&0xffff)!=PACKET_ENC_SURF) throw Error::ParseError();
	// Run checksum for entire packet...
	bool crcPacket=InternalCRC((unsigned char*)ptr);
	// Loop over all waveforms in encoded surf packet and create packets for
	// individual waveforms
	int offset=sizeof(EncodedSurfPacketHeader_t);
	packets.reserve(packets.size()+CHANNELS_PER_SURF);
	for(int i=0;i<CHANNELS_PER_SURF;++i){
	  EncodedSurfChannelHeader_t *chanHead=(EncodedSurfChannelHeader_t*)((unsigned char*)ptr+offset);
	  // Create regular waveform packet and fill it
	  try{
	    RawWaveformPacket_t wv;
	    wv.gHdr.code=esurf_p->gHdr.code;
	    wv.gHdr.packetNumber=esurf_p->gHdr.packetNumber;	
	    wv.gHdr.numBytes=sizeof(RawWaveformPacket_t);
	    wv.gHdr.feByte=esurf_p->gHdr.feByte;
	    wv.gHdr.verId=esurf_p->gHdr.verId;
	    wv.gHdr.checksum=0; // No checksum
	    wv.eventNumber=esurf_p->eventNumber;
	    // Run checksum for encoded waveform only
	    int crcPacketIndividual=crcPacket+(((int)WaveformCRC((unsigned char*)ptr+offset))<<1);
	    // Decompress waveform
	    unsigned char *encdata=(unsigned char*)ptr+offset+sizeof(EncodedSurfChannelHeader_t);
	    CompressErrorCode_t retVal=decodeChannel(chanHead,encdata,&wv.waveform);
	    if(retVal==COMPRESS_E_BAD_CODE)
	      throw Error::UnknownEncodingType((unsigned char *)chanHead,
					       sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes,
					       esurf_p->eventNumber,(int)chanHead->encType);
	    if(retVal!=COMPRESS_E_OK) 
	      throw Error::DecodingError((unsigned char *)chanHead,
					 sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes,
					 esurf_p->eventNumber,(int)retVal);
	    // Parse new packet
	    packets.push_back(Packet((void*)(&wv),wv.gHdr.numBytes,type,crcWrapper,crcPacketIndividual));
	  }
	  catch(Error::BaseError &e){
	    cerr << e.message << " " << e.action << endl;
	    if(e.info[0]) cerr << e.info << endl;
	  }
	  offset+=sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes;
	}
	if(offset!=esurf_p->gHdr.numBytes)
	  cerr << "Offset byte mismatch in processing of encoded surf packet; "
	       << "declared " << esurf_p->gHdr.numBytes << " counted " 
	       << offset << " packet number = " <<  esurf_p->gHdr.packetNumber << endl;
	size=offset; // Account for channel header and waveforms in data stream
	break;
      }
      case TYPE_ENC_SURF_PEDSUB:{// This is encoded ped subbed surf packets so we need to do some monkeying 
	EncodedPedSubbedSurfPacketHeader_t *esurf_p=(EncodedPedSubbedSurfPacketHeader_t*)ptr;
	if((esurf_p->gHdr.code&0xffff)!=PACKET_ENC_SURF_PEDSUB) throw Error::ParseError();
	// Run checksum for entire packet...
	bool crcPacket=InternalCRC((unsigned char*)ptr);
	// Loop over all waveforms in encoded surf packet and create packets for
	// individual waveforms
	int offset=sizeof(EncodedPedSubbedSurfPacketHeader_t);
	packets.reserve(packets.size()+CHANNELS_PER_SURF);
	for(int i=0;i<CHANNELS_PER_SURF;++i){ 
	  EncodedSurfChannelHeader_t *chanHead=(EncodedSurfChannelHeader_t*)((unsigned char*)ptr+offset);
	  // Create regular waveform packet and fill it
	  try{
	    PedSubbedWaveformPacket_t wv;
	    wv.gHdr.code=esurf_p->gHdr.code;
	    wv.gHdr.packetNumber=esurf_p->gHdr.packetNumber;	
	    wv.gHdr.numBytes=sizeof(PedSubbedWaveformPacket_t);
	    wv.gHdr.feByte=esurf_p->gHdr.feByte;
	    wv.gHdr.verId=esurf_p->gHdr.verId;
	    wv.gHdr.checksum=0; // No checksum
	    wv.eventNumber=esurf_p->eventNumber;
	    wv.whichPeds=esurf_p->whichPeds;
	    // Run checksum for encoded waveform only
	    int crcPacketIndividual=crcPacket+(((int)WaveformCRC((unsigned char*)ptr+offset))<<1);
	    // Decompress waveform
	    unsigned char *encdata=(unsigned char*)ptr+offset+sizeof(EncodedSurfChannelHeader_t);
	    CompressErrorCode_t retVal=decodePSChannel(chanHead,encdata,&wv.waveform);
	    if(retVal==COMPRESS_E_BAD_CODE)
	      throw Error::UnknownEncodingType((unsigned char*)chanHead,
					       sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes,
					       esurf_p->eventNumber,(int)chanHead->encType);
	    if(retVal!=COMPRESS_E_OK) 
	      throw Error::DecodingError((unsigned char*)chanHead,
					 sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes,
					 esurf_p->eventNumber,(int)retVal);
	    // Parse new packet
	    packets.push_back(Packet((void*)(&wv),wv.gHdr.numBytes,type,crcWrapper,crcPacketIndividual));
	  }
	  catch(Error::BaseError &e){
	    cerr << e.message << " " << e.action << endl;
	    if(e.info[0]) cerr << e.info << endl;
	  }
	  offset+=sizeof(EncodedSurfChannelHeader_t)+chanHead->numBytes;
	}
	if(offset!=esurf_p->gHdr.numBytes)
	  cerr << "Offset byte mismatch in processing of encoded ped subbed surf packet; "
	       << "declared " << esurf_p->gHdr.numBytes << " counted " 
	       << offset << " packetNumber = " << esurf_p->gHdr.packetNumber << endl;
	size=offset; // Account for channel headers and waveforms in data stream
	break;
      }
      case TYPE_LAB_PED:{ /* Pedestal data, one structure contains pedestals for entire chip
			     So, send it CHANNELS_PER_SURF times, encoding which channel to decode
			     in upper bytes of packet code. */
	long *code=(long*)ptr;
	packets.reserve(packets.size()+CHANNELS_PER_SURF);
	for(int i=0;i<CHANNELS_PER_SURF;++i){
	  (*code)=((*(long*)ptr)&0xffff)+(i<<16); // PacketCode_t is long, but values are always <0xffff, so we can safely store an integer in the upper two bytes since checksum on whole packet was already performed
	  try{
	    packets.push_back(Packet(ptr,size,type,crcWrapper));
	  }
	  catch(Error::BaseError &e){
	    cerr << e.message << " " << e.action << endl;
	    if(e.info[0]) cerr << e.info << endl;
	  }
	}
	break;
      }
      case TYPE_ZIPPED_PACKET:   // This is zipped packet, so size needs to be corrected
      case TYPE_ZIPPED_FILE:     // This is zipped file, so size needs to be corrected
	size=((GenericHeader_t*)ptr)->numBytes; // Override size with the internal number
	// Let it fall through for regular processing
      default:
	packets.push_back(Packet(ptr,size,type,crcWrapper));
      }
    }
    catch(Error::BaseError &e){  // Catch all remaining throws here, so that any data after error can get 
                                 // a chance to be processed
      fprintf(stderr,"%s %s\n",e.message,e.action);
      if(e.info[0]) fprintf(stderr,"%s\n",e.info);
    }
    
      //cerr<<"XferPacket:size = "<< size << endl;//peng
    nProcessed+=size;  // Increment processed byte counter
  }
  
  return;
};

void XferPacket::store(PGconn *DBconn){
  // Send all packets individually
  //for(unsigned int i=0;i<packets.size();++i) packets[i].store(DBconn);

  // Send all packet at once 
  string sqlcmd;
//  sqlcmd="BEGIN;\n";
  for(int i=0;i<packets.size();++i){
    sqlcmd+=packets[i].sqlCommand();
    sqlcmd+="\n";
  }
//  sqlcmd+="COMMIT;\n";
//  std::cout<<sqlcmd<<std::endl;
//  cerr<<sqlcmd<<std::endl;

  // Execute SQL packet insertion
//  cerr<<"ddm: sending big transaction"<<endl;
  PGresult *res=PQexec(DBconn,sqlcmd.c_str());
  // Verify result, very simplistic for now
  int code=PQresultStatus(res);
  switch(code){
  case PGRES_COMMAND_OK:
//    cerr<<"ddm: transaction was ok"<<endl;
    break;
  default:
    //throw Error::DBError(sqlcmd.c_str(),code);
    // On failure, try sending individually
//    cerr<<"ddm: transaction was rejected with code "<<code<<endl;
    char *sqlState=PQresultErrorField(res,PG_DIAG_SQLSTATE);
//    cerr<<"ddm: errorField: "<<sqlState<<endl;
//    cerr<<"ddm: error in sending packets to the DB, ROLLBACK transaction and sending individually"<<endl;
//    PQexec(DBconn,"ROLLBACK;\n");
    for(unsigned int i=0;i<packets.size();++i) packets[i].store(DBconn);
//    cerr<<"ddm: done sending individual packets"<<endl;
  }
  PQclear(res);
  return;
}

bool XferPacket::WaveformCRC(unsigned char *ptr){
  EncodedSurfChannelHeader_t *chanHead=(EncodedSurfChannelHeader_t*)ptr;
  unsigned short wvcrc=crc_short((unsigned short*)(ptr+sizeof(EncodedSurfChannelHeader_t)),
				 chanHead->numBytes/sizeof(short));
  bool crcCheck=(wvcrc==chanHead->crc);
  if(!crcCheck)
    cerr << "Bad waveform CRC, old " << hex << chanHead->crc
	 << " new " << wvcrc << " encoding type = " << chanHead->encType
      // << dec << " channel id = " << chanHead->rawHdr.chanId << endl;	//original code 
         << " channel id = " << int(chanHead->rawHdr.chanId) << endl;	// amir's modification
  return crcCheck;
}

// Packet constructor, strips data common to all 
Packet::Packet(unsigned char *data,int n)
  :pckStruct_p(NULL),nData(0),fake_pp(NULL){
  DataType_t type;
  bool crcWrapper=checkWrapper(data,n,&type);
  parsePacket(type,crcWrapper);
  return;
};

// Alternate constructor
Packet::Packet(void *ptr,int n,DataType_t type,bool crcWrapper,int crcInternal)
  :pckStruct_p(ptr),nData(n),fake_pp(NULL){
  parsePacket(type,crcWrapper,crcInternal);
  return;
};

// Copy constructor
Packet::Packet(const Packet &old){
  pckStruct_p=old.pckStruct_p;
  nData=old.nData;
  fields=old.fields;
  table=old.table;
  if(old.fake_pp){
    fake_pp=(void*)(new unsigned char[nData]);
    memcpy(fake_pp,old.fake_pp,nData);
  }else
    fake_pp=NULL;
}

//Destructor
Packet::~Packet(){
  fields.clear();
  if(fake_pp) delete[] ((unsigned char*)fake_pp);
}

void Packet::clear(){
  pckStruct_p=NULL;
  nData=0;
  fields.clear();
  table=""; 
  if(fake_pp){
    delete[] ((unsigned char*)fake_pp);
    fake_pp=NULL;
  }
}

bool Packet::InternalCRC(unsigned char *ptr){
  GenericHeader_t *gHdr=(GenericHeader_t*)ptr;
  //unsigned long newcrc=crc_long((unsigned long*)(ptr+sizeof(GenericHeader_t)),
  //				(gHdr->numBytes-sizeof(GenericHeader_t))/sizeof(long));
  unsigned int newcrc=crc_int((unsigned int*)(ptr+sizeof(GenericHeader_t)),
				(gHdr->numBytes-sizeof(GenericHeader_t))/sizeof(int));
  bool crcCheck=(newcrc==gHdr->checksum);
  if(!crcCheck)
    cerr << "Bad internal CRC, old " << hex << gHdr->checksum 
	 << " new " << newcrc << " packet type = " << gHdr->code 
	 << dec << " packet number = " << gHdr->packetNumber << endl;
  return crcCheck;
}

void Packet::parsePacket(DataType_t type,bool crcWrapper, int crcInternal){
  if(!pckStruct_p) throw Error::NullStructure();

  // Here's the handling of zipped packets. 
  // BIG WARNING!! For regular non-zipped packets pckStruct_p points to the place in the
  // memory handled by lddl library which contains packet data. For packet we unzip, the
  // memory will be allocated for the unzipped copy (handled by fake_pp variable), and then 
  // we point pckStruct_p to memory pointed to by fake_pp. Hopefully, this causes no ill 
  // effects, but you have been warned.
  if((*((PacketCode_t*)pckStruct_p))==PACKET_ZIPPED_PACKET){
    cerr<<"this is a zipped packet"<<endl;
    ZippedPacket_t *zip_p=(ZippedPacket_t*)pckStruct_p;
    unsigned long unzippedBytes=zip_p->numUncompressedBytes;
    int zippedBytes=zip_p->gHdr.numBytes-sizeof(ZippedPacket_t);
    fake_pp=(void*)(new unsigned char[unzippedBytes]);
    int retVal=uncompress((Bytef*)fake_pp,(uLongf*)&unzippedBytes,(const Bytef*)((const char*)zip_p+sizeof(ZippedPacket_t)),zippedBytes);
    if(retVal!=Z_OK || unzippedBytes!=zip_p->numUncompressedBytes)
      throw Error::UnzipError((unsigned char *)pckStruct_p,nData);
    GenericHeader_t *hdr=(GenericHeader_t*)fake_pp;
    hdr->packetNumber=zip_p->gHdr.packetNumber+(1<<30); // Mark that this packet came from a zipped parent
    // Here we override original memory location and length of packet data
    pckStruct_p=fake_pp;
    nData=unzippedBytes;
  }
  
  // First strip and pack common fields
  GenericHeader_t *gHdr=(GenericHeader_t*)pckStruct_p;
  if(gHdr->feByte!=0xfe) 
    cerr << "Warning: feByte is " << hex << (int)gHdr->feByte << dec << endl; 
  unsigned long nBuf=gHdr->packetNumber;
  if(type==SIPHR) nBuf+=(1<<31); // Set highest bit to mark that it came over TDRSS
  // Check lengths 
  //cerr << "packetNumber: " << gHdr->packetNumber <<endl;
  if(gHdr->numBytes!=nData)
    cerr << "Inconsistent packet length description; external " << nData
	 << " internal " << gHdr->numBytes 
	 << " (code " << hex << gHdr->code << ")" << dec << endl;
  // Check internal CRC
  if(type!=SIPLR && crcInternal<0) crcInternal=InternalCRC((unsigned char*)pckStruct_p);
  
  if(type!=SIPLR && (!crcInternal && crcWrapper))
    cerr << "Funny! Internal CRC bad, external CRC good; packet type " 
	 << hex << (*((PacketCode_t*)pckStruct_p)) << dec 
	 << "; buf number " << nBuf << endl;
  unsigned short crc=(crcWrapper<<8)+crcInternal;
  //cerr << "crc=" << crc << " crcWrapper="<< (crcWrapper) << " crcInternal=" <<crcInternal<<" pckStruct_p="<<((*((PacketCode_t*)pckStruct_p))&0xffff)<<endl;
  
  fields.push_back(Word(nBuf,"nbuf"));
  fields.push_back(Word(crc,"crc"));
  fields.push_back(Word(time(NULL),"now"));
  try{
    switch((*((PacketCode_t*)pckStruct_p))&0xffff){ // Mask on lower bytes, since upper bytes 
                                                    // may contain some additional info inserted locally
    case PACKET_HD: parseHd(); break;
    case PACKET_WV: 
    case PACKET_SURF:
    case PACKET_ENC_SURF:  parseWv(); break;  // These are raw waveforms
    case PACKET_PEDSUB_WV: 
    case PACKET_ENC_WV_PEDSUB:
    case PACKET_PEDSUB_SURF:
    case PACKET_ENC_SURF_PEDSUB: parsePsWv(); break; // These are pedsubbed waveforms
    case PACKET_LAB_PED: parsePed(); break;
    case PACKET_SURF_HK: parseSurfHk(); break;
    case PACKET_TURF_RATE: parseTurf(); break;
    case PACKET_RTLSDR_POW_SPEC: parseRtlsdr(); break;
    case PACKET_TUFF_STATUS: parseTuffStatus(); break;
    case PACKET_TUFF_RAW_CMD: parseTuffCmd(); break;
    case PACKET_GPS_ADU5_PAT: parseAdu5Pat((*(unsigned int*)pckStruct_p)&0xffff0000); break;
    case PACKET_GPS_ADU5_VTG: parseAdu5Vtg((*(unsigned int*)pckStruct_p)&0xffff0000); break;
    case PACKET_GPS_ADU5_SAT: parseAdu5Sat((*(unsigned int*)pckStruct_p)&0xffff0000); break;
    case PACKET_GPS_G12_POS: parseG12Pos(); break;
    case PACKET_GPS_G12_SAT: parseG12Sat(); break;
    case PACKET_HKD: parseHkd(); break;
    case PACKET_HKD_SS: parseSSHkd(); break;
    case PACKET_CMD_ECHO: parseCmd(); break;
    case PACKET_MONITOR: parseMonitor(); break;
    case PACKET_WAKEUP_LOS: parseWakeupLOS(); break;
    case PACKET_WAKEUP_HIGHRATE: parseWakeupHiRate(); break;
    case PACKET_WAKEUP_COMM1: parseWakeupCOMM1(); break;
    case PACKET_WAKEUP_COMM2: parseWakeupCOMM2(); break;
    case PACKET_SLOW_FULL: parseSlowFull(); break;
    case PACKET_ZIPPED_FILE: parseFile(); break;
    case PACKET_RUN_START: parseRun(); break; 
    case PACKET_OTHER_MONITOR: parseOtherMonitor(); break; 
    case PACKET_GPS_GGA: /* FIXME  ddm peng */ break; 
  case PACKET_AVG_SURF_HK: /* FIXME  ddm peng */ break; 
  case PACKET_SUM_TURF_RATE: /* FIXME  ddm peng */ break; 
  case PACKET_GPU_AVE_POW_SPEC: /* FIXME  ddm peng */ break; 
    default:
      throw Error::UnmatchedPacketType((unsigned char*)pckStruct_p,nData,*((PacketCode_t*)pckStruct_p));
    }

  }
  catch(Error::ParseError){
    if ((*((PacketCode_t*)pckStruct_p)&0xffff) == PACKET_GPS_ADU5_VTG)
      printf(" adu5_vtg rejected as parse error.\n") ;
    throw Error::FailedInitialization((unsigned char*)pckStruct_p,nData);
  }
		   
  return;
}
  
bool Packet::checkWrapper(unsigned char *data,int n,DataType_t *type){
  if(n<1) throw Error::EmptyLine();  // Needs to have data
  // Decide on data format and check minimum length
  void*(*depack)(unsigned char*,int,unsigned short*);
  int(*good)(unsigned char*,int);

  if((*(unsigned short*)(data+6))==LOS_MASK){  // LOS MASK repeats 
    *type=LOS;
    depack=&depackLOS;
    good=&goodLOS;
    if(n<24) throw Error::MalformatedPacket(data,n); // Length of wrapper only
  }else if((*(unsigned short*)data)==SIPHR_MASK) {
    //                                     14-Jul-08  SM
    // according to Marty, LOD data format is the same as TDRSS now.
    // only difference is 0th bit of ID_HDR.  
    if (*(unsigned short*)(data+4) & 1) *type=SIPHR;
    else *type=LOS ;
    //if (*type==LOS)       fprintf(stderr,"SIPHR, but it's LOS data!\n");
//     *type=SIPHR;
    depack=&depackSIPhr;
    good=&goodSIPhr;
    if(n<18) throw Error::MalformatedPacket(data,n); // Length of wrapper only
  }else if(*data==0xC1 || *data==0xC2){
    *type=SIPLR;
    depack=&depackSIPlr;
    good=&goodSIPlr;
    if(n<3) throw Error::MalformatedPacket(data,n); // Length of wrapper only
  }else
    throw Error::MalformatedPacket(data,n); // Unknown format!

  // Check packet wrapper quality
  // Don't throw out if only wrapper is bad, data might be good
  //if(!good(data,n)) throw Error::MalformatedPacket(data,n); 
  if(!good(data,n)){
    cerr << "Bad wrapper! " << endl;
  }

  // Strip wrapper info and check CRC
  unsigned int nBuf;
  unsigned int crcOld,crcNew;
  wrapper(*type,data,n,&nBuf,&crcOld);
  pckStruct_p=depack(data,n,&nData);

  if(!pckStruct_p) throw Error::ParseError();
  switch(*type){
  case LOS:
    // (nData+1)/2 construct will round up in case there is odd number of science bytes
    crcNew=crc_short((unsigned short*)pckStruct_p,(nData+1)/2);
    // For some reason Marty is running checksum starting at word 6, i.e. 
    // over extra 6 bytes
    //crcNew=crc_short((unsigned short*)(data+12),nData/2+3); 
    // Now in another set of data, it runs checksum over 6 bytes too few...
    //crcNew=crc_short((unsigned short*)(data+24),nData/2-3); 
    break;
  case SIPHR:
    crcNew=crc_short((unsigned short *)pckStruct_p,(nData+1)/2);
    break;
  case SIPLR:
    crcNew=crcOld;
    break;
  }
  
  if(crcNew!=crcOld){
    cerr << "Bad CRC! old: " << hex << crcOld << " new: " << crcNew << dec
	 << " Buffer count: " << getLSBlong(data+12) << endl;
    // Don't throw out bad CRC...
    //throw Error::CRCError(data,n);
  }
  return (crcNew==crcOld);
}

/* Creates SQL commad for storing this packet data into database */
string Packet::sqlCommand(){
   // Create a string for SQL command
  ostringstream sqlcmd;
  sqlcmd.precision(12);
  sqlcmd.width(12);

  // Prepare SQL command
  if(!table.empty()){
		if(table == "wv") {
		//cerr<<"ddm: in wv branch, using execute"<<endl;
			// ddm
			// wv table, use prepared insert
			sqlcmd << "EXECUTE ddmwv (";
			// Loop over all fields and record their values
			for(unsigned int i=0;i<fields.size();++i){
				sqlcmd << fields[i];
				if(i<fields.size()-1) sqlcmd << ",";
			}
			sqlcmd << ");";
			return sqlcmd.str();
		}
    sqlcmd << "INSERT INTO " << table << " (";
    // Loop over all fields and record their names
    for(unsigned int i=0;i<fields.size();++i){
      sqlcmd << "\"" << fields[i].getTag() << "\"";
      if(i<fields.size()-1) sqlcmd << ",";
    }
    sqlcmd << ") VALUES (";
    // Loop over all fields and record their values
    for(unsigned int i=0;i<fields.size();++i){
      sqlcmd << fields[i];
      if(i<fields.size()-1) sqlcmd << ",";
    }
    
    sqlcmd << ");";
  }
  return sqlcmd.str();
}

/* Stores packet into database */
void Packet::store(PGconn *DBconn){
  string sqlcmd=sqlCommand();
  //  cerr << sqlcmd.c_str() << endl;
  if(sqlcmd.empty()) return;

  // Execute SQL packet insertion
//  cerr<<"ddm: sending single packet"<<endl;
  PGresult *res=PQexec(DBconn,sqlcmd.c_str());
  // Verify result, very simplistic for now
   int code=PQresultStatus(res);
   switch(code){
   case PGRES_COMMAND_OK:
     break;
   case PGRES_FATAL_ERROR:{
     char *sqlState=PQresultErrorField(res,PG_DIAG_SQLSTATE);
//     cerr<<"ddm: error in sending individual packet. Code: "<<sqlState<<endl;
     // If uniqueness violation, don't report the error
     if(!strcmp(sqlState,"23505")) break;
     //ddm: print out diag message for all errors
     //if(!strcmp(sqlState,"XX000"))
       cerr << PQresultErrorField(res,PG_DIAG_MESSAGE_PRIMARY) << ";"; 
   }
   default:
     throw Error::DBError(sqlcmd.c_str(),code);
     cerr << "Failed to insert\n";
     cerr << sqlcmd.c_str() << endl;
   }
//   cerr<<"ddm: done with single packet"<<endl;
   PQclear(res);
   return;
}

//Houskeeping parser
void Packet::parseHkd(){
  // Set DB table name, create pointer to data structure and check type again
  table="hk";
  //  printf(" hk packet.\n") ;
  if(nData!=sizeof(HkDataStruct_t)) throw Error::ParseError();
  HkDataStruct_t *hkd_p=(HkDataStruct_t*)pckStruct_p;
  if((hkd_p->gHdr.code&0xffff)!=PACKET_HKD) throw Error::ParseError();

  // Store values
  fields.reserve(14);
  fields.push_back(Word(hkd_p->unixTime,"time"));
  fields.push_back(Word(hkd_p->unixTimeUs,"us"));
  fields.push_back(Word(hkd_p->ip320.code,"code"));
  for(int b=0;b<NUM_IP320_BOARDS;++b){
    ostringstream bdarray;
    bdarray << "{";
    ostringstream bd;
    bd << "bd" << b+1;
    for(int i=0;i<CHANS_PER_IP320;++i){
      unsigned short val=hkd_p->ip320.board[b].data[i];
      bdarray << (val>>4);   // Use upper 12 bits 
      if(i<CHANS_PER_IP320-1) bdarray << ",";
    }
    bdarray << "}";
    fields.push_back(Word(bdarray.str().c_str(),bd.str().c_str()));
  }
  fields.push_back(Word(hkd_p->mag.x,"magx"));
  fields.push_back(Word(hkd_p->mag.y,"magy"));
  fields.push_back(Word(hkd_p->mag.z,"magz"));
  // I'm not sure whether this is a good place for conversion or not.
  // It will be here for now. add 15 for core temp, after Patrick.
  // This may move to AV.         SM.   11-Aug-08  
  fields.push_back(Word(hkd_p->sbs.temp[0],"sbst1"));
  fields.push_back(Word(hkd_p->sbs.temp[1],"sbst2"));
  fields.push_back(Word(hkd_p->sbs.temp[2],"core1"));
  fields.push_back(Word(hkd_p->sbs.temp[3],"core2"));
  fields.push_back(Word(hkd_p->sbs.temp[4],"sbst5"));//peng
  fields.push_back(Word(hkd_p->sbs.temp[5],"sbst6"));//peng

  return;
}

//TODO remove later , guess SS hk data code.
AnalogueCode_t Packet::guessCode(SSHkDataStruct_t *hkPtr) {
  float mean=0;
  for(int i=0;i<CHANS_PER_IP320;i++) {
        mean+=((hkPtr->ip320.board.data[i])>>4);
  }
  mean/=CHANS_PER_IP320;
  if(mean>4000) return IP320_CAL;
  if(mean<2050) return IP320_AVZ;
  return IP320_RAW;
}


//Sun Sensor Houskeeping parser/////peng
void Packet::parseSSHkd(){
  // Set DB table name, create pointer to data structure and check type again
  table="SShk";
  if(nData!=sizeof(SSHkDataStruct_t)) throw Error::ParseError();
  SSHkDataStruct_t *sshkd_p=(SSHkDataStruct_t*)pckStruct_p;
//cerr<< "ssHkd:: SingleAnalogueStruct_t -> code ="<<sshkd_p->ip320.code<<endl;
  if((sshkd_p->gHdr.code&0xffff)!=PACKET_HKD_SS) throw Error::ParseError();
sshkd_p->ip320.code=guessCode(sshkd_p);//TODO remove later
  // Store values
  fields.reserve(14);
  fields.push_back(Word(sshkd_p->unixTime,"time"));
  fields.push_back(Word(sshkd_p->unixTimeUs,"us"));
  fields.push_back(Word(sshkd_p->ip320.code,"code"));
    ostringstream bdarray;
    bdarray << "{";
    for(int i=0;i<CHANS_PER_IP320;++i){
      unsigned short val=sshkd_p->ip320.board.data[i];
      bdarray << (val>>4);   // Use upper 12 bits 
      if(i<CHANS_PER_IP320-1) bdarray << ",";
    }
    bdarray << "}";
  fields.push_back(Word(bdarray.str().c_str(),"bd1"));
  return;
}



//ADU5 GPS pattern parser
void Packet::parseAdu5Pat(unsigned int gpstype){
  // Set DB table name, create pointer to data structure and check type again
  table="adu5_pat";
  //  printf(" adu5_pat packet.\n") ;
  if(nData!=sizeof(GpsAdu5PatStruct_t)) throw Error::ParseError();
  GpsAdu5PatStruct_t *gps_p=(GpsAdu5PatStruct_t*)pckStruct_p;
  if((gps_p->gHdr.code&0xffff)!=PACKET_GPS_ADU5_PAT) throw Error::ParseError();
  //  printf(" processing adu5_pat packet. %x %d\n", *gps_p, gps_p->unixTime) ;

  // Store values
  fields.reserve(18);
  fields.push_back(Word(gpstype,"gpstype"));
  fields.push_back(Word(gps_p->gHdr.code,"code"));
  fields.push_back(Word(gps_p->unixTime,"time"));
  fields.push_back(Word(gps_p->unixTimeUs,"us"));
  fields.push_back(Word(gps_p->timeOfDay,"tod"));
  fields.push_back(Word(gps_p->heading,"heading"));
  fields.push_back(Word(gps_p->pitch,"pitch"));
  fields.push_back(Word(gps_p->roll,"roll"));
  fields.push_back(Word(gps_p->mrms,"mrms"));
  fields.push_back(Word(gps_p->brms,"brms"));
  fields.push_back(Word(gps_p->attFlag,"flag"));
  fields.push_back(Word(gps_p->latitude,"latitude"));
  fields.push_back(Word(gps_p->longitude,"longitude"));
  fields.push_back(Word(gps_p->altitude,"altitude"));

  return;
}

//ADU5 GPS velocity parser
void Packet::parseAdu5Vtg(unsigned int gpstype){
  // Set DB table name, create pointer to data structure and check type again
  table="adu5_vtg";
  //  printf(" adu5_vtg packet.\n") ;
  if(nData!=sizeof(GpsAdu5VtgStruct_t)) throw Error::ParseError();
  GpsAdu5VtgStruct_t *gps_p=(GpsAdu5VtgStruct_t*)pckStruct_p;
  if((gps_p->gHdr.code&0xffff)!=PACKET_GPS_ADU5_VTG) throw Error::ParseError();
  //  printf(" processing adu5_vtg packet. %x %ld\n", *gps_p, gps_p->unixTime);

  // Store values
  fields.reserve(12);
  fields.push_back(Word(gpstype,"gpstype"));
  fields.push_back(Word(gps_p->gHdr.code,"code"));
  fields.push_back(Word(gps_p->unixTime,"time"));
  fields.push_back(Word(gps_p->unixTimeUs,"us"));
  fields.push_back(Word(gps_p->trueCourse,"course"));
  fields.push_back(Word(gps_p->magneticCourse,"mcourse"));
  fields.push_back(Word(gps_p->speedInKnots,"vkt"));
  fields.push_back(Word(gps_p->speedInKPH,"vkph"));
  return;
}

//ADU5 GPS satellite info parser
void Packet::parseAdu5Sat(unsigned int gpstype){
  // Set DB table name, create pointer to data structure and check type again
  table="adu5_sat";
  //  printf(" adu5_sat packet.\n") ;
  if(nData!=sizeof(GpsAdu5SatStruct_t)) throw Error::ParseError();
  GpsAdu5SatStruct_t *gps_p=(GpsAdu5SatStruct_t*)pckStruct_p;
  if((gps_p->gHdr.code&0xffff)!=PACKET_GPS_ADU5_SAT) throw Error::ParseError();
  //  printf(" processing adu5_sat packet. %x\n", *gps_p) ;

  // Store values
  fields.reserve(10);
  fields.push_back(Word(gpstype,"gpstype"));
  fields.push_back(Word(gps_p->gHdr.code,"code"));
  fields.push_back(Word(gps_p->unixTime,"time"));

  int maxsat=1; // Need one in order not to have empty arrays
  for(int i=0;i<4;++i) if(maxsat<gps_p->numSats[i]) maxsat=gps_p->numSats[i];
  ostringstream numsats,prn,elevation,snr,flag,azimuth;  
  numsats << "{";
  prn << "{";
  elevation << "{";
  snr << "{";
  flag << "{";
  azimuth << "{";
  for(int i=0;i<4;++i){
    int s;
    numsats << (short)(gps_p->numSats[i]);
    prn << "{";
    elevation << "{";
    snr << "{";
    flag << "{";
    azimuth << "{";
    for(s=0;s<gps_p->numSats[i];++s){
      prn << (short)(gps_p->sat[i][s].prn);
      elevation << (short)(gps_p->sat[i][s].elevation);
      snr << (short)(gps_p->sat[i][s].snr);
      flag << (short)(gps_p->sat[i][s].flag);
      azimuth << gps_p->sat[i][s].azimuth;
      if(s<maxsat-1){
	prn << ",";
	elevation << ",";
	snr << ",";
	flag << ",";
	azimuth << ",";
      }
    }
    for(;s<maxsat;++s){ // Fill in if needed
      prn << "-1";
      elevation << "-1";
      snr << "-1";
      flag << "-1";
      azimuth << "-1";
      if(s<maxsat-1){
	prn << ",";
	elevation << ",";
	snr << ",";
	flag << ",";
	azimuth << ",";
      }
    }
    prn << "}";
    elevation << "}";
    snr << "}";
    flag << "}";
    azimuth << "}";
    if(i<3){
      prn << ",";
      elevation << ",";
      snr << ",";
      flag << ",";
      azimuth << ",";
      numsats << ",";
    }
  }
  numsats << "}";
  prn << "}";
  elevation << "}";
  snr << "}";
  flag << "}";
  azimuth << "}";

  fields.push_back(Word(numsats.str().c_str(),"numsats"));
  fields.push_back(Word(prn.str().c_str(),"prn"));
  fields.push_back(Word(elevation.str().c_str(),"elevation"));
  fields.push_back(Word(snr.str().c_str(),"snr"));
  fields.push_back(Word(flag.str().c_str(),"flag"));
  fields.push_back(Word(azimuth.str().c_str(),"azimuth"));

  return;
}

//G12 GPS position parser
void Packet::parseG12Pos(){
  // Set DB table name, create pointer to data structure and check type again
  table="g12_pos";
  //  printf(" g12_pos packet.\n") ;
  if(nData!=sizeof(GpsG12PosStruct_t)) throw Error::ParseError();
  GpsG12PosStruct_t *gps_p=(GpsG12PosStruct_t*)pckStruct_p;
  if((gps_p->gHdr.code&0xffff)!=PACKET_GPS_G12_POS) throw Error::ParseError();

  // Store values
  fields.reserve(17);
  fields.push_back(Word(gps_p->unixTime,"time"));
  fields.push_back(Word(gps_p->unixTimeUs,"us"));
  fields.push_back(Word(gps_p->timeOfDay,"tod"));
  fields.push_back(Word(gps_p->numSats,"numsats"));
  fields.push_back(Word(gps_p->latitude,"latitude"));
  fields.push_back(Word(gps_p->longitude,"longitude"));
  fields.push_back(Word(gps_p->altitude,"altitude"));
  fields.push_back(Word(gps_p->trueCourse,"course"));
  fields.push_back(Word(gps_p->verticalVelocity,"upv"));
  fields.push_back(Word(gps_p->speedInKnots,"vkt"));
  fields.push_back(Word(gps_p->pdop,"pdop"));
  fields.push_back(Word(gps_p->hdop,"hdop"));
  fields.push_back(Word(gps_p->vdop,"vdop"));
  fields.push_back(Word(gps_p->tdop,"tdop"));
  //added to deal with ADU5 unit number
  fields.push_back(Word((gps_p->gHdr.code&0x70000)>>16,"unit"));

  return;
}

//G12 GPS satellite info parser
void Packet::parseG12Sat(){
  // Set DB table name, create pointer to data structure and check type again
  table="g12_sat";
  if(nData!=sizeof(GpsG12SatStruct_t)) throw Error::ParseError();
  GpsG12SatStruct_t *gps_p=(GpsG12SatStruct_t*)pckStruct_p;
  if((gps_p->gHdr.code&0xffff)!=PACKET_GPS_G12_SAT) throw Error::ParseError();

  // Store values
  fields.reserve(10);
  fields.push_back(Word(gps_p->unixTime,"time"));
  fields.push_back(Word(gps_p->numSats,"numsats"));
  ostringstream prn,elevation,snr,flag,azimuth;
  prn << "{";
  elevation << "{";
  snr << "{";
  flag << "{";
  azimuth << "{";
  for(unsigned int s=0;s<gps_p->numSats;++s){
    prn << (short)(gps_p->sat[s].prn);
    elevation << (short)(gps_p->sat[s].elevation);
    snr << (short)(gps_p->sat[s].snr);
    flag << (short)(gps_p->sat[s].flag);
    azimuth << gps_p->sat[s].azimuth;
    if(s<gps_p->numSats-1){
      prn << ",";
      elevation << ",";
      snr << ",";
      flag << ",";
      azimuth << ",";
    }
  }
  prn << "}";
  elevation << "}";
  snr << "}";
  flag << "}";
  azimuth << "}";

  fields.push_back(Word(prn.str().c_str(),"prn"));
  fields.push_back(Word(elevation.str().c_str(),"elevation"));
  fields.push_back(Word(snr.str().c_str(),"snr"));
  fields.push_back(Word(flag.str().c_str(),"flag"));
  fields.push_back(Word(azimuth.str().c_str(),"azimuth"));

  return;
}

//Event header parser
void Packet::parseHd(){
  // Set DB table name, create pointer to data structure and check type again
  table="hd";
  //  printf(" hd packet.\n") ;
  if(nData!=sizeof(AnitaEventHeader_t)) throw Error::ParseError();
  AnitaEventHeader_t *hd_p=(AnitaEventHeader_t*)pckStruct_p;
  if((hd_p->gHdr.code&0xffff)!=PACKET_HD) throw Error::ParseError();

  // Store values
  fields.reserve(22);
  fields.push_back(Word(hd_p->unixTime,"time"));
  fields.push_back(Word(hd_p->unixTimeUs,"us"));
  fields.push_back(Word(hd_p->gpsSubTime,"ns"));
  fields.push_back(Word(hd_p->turfEventId,"evid"));
  fields.push_back(Word(hd_p->eventNumber,"evnum"));
  //*  fields.push_back(Word(hd_p->surfMask,"surfmask"));
  fields.push_back(Word(hd_p->calibStatus,"calib"));
  fields.push_back(Word(hd_p->priority,"priority"));
  fields.push_back(Word(hd_p->turfUpperWord,"turfword"));
  fields.push_back(Word(hd_p->l2TrigMask,"l1mask"));// intentionally remain the l1mask, to compatiable with anita3 , peng
  fields.push_back(Word(hd_p->l2TrigMaskH,"l1maskh"));//peng
  fields.push_back(Word(hd_p->phiTrigMask,"phimask"));
  fields.push_back(Word(hd_p->phiTrigMaskH,"phimaskh"));//peng
  fields.push_back(Word(hd_p->peakThetaBin,"peakthetabin"));
  fields.push_back(Word(hd_p->imagePeak,"imagepeak"));
  fields.push_back(Word(hd_p->coherentSumPeak,"coherentsumpeak"));
  fields.push_back(Word(hd_p->prioritizerStuff,"prioritizerstuff"));
  fields.push_back(Word(hd_p->turfio.trigType,"trigtype"));
  fields.push_back(Word(hd_p->turfio.trigNum,"trignum"));
  fields.push_back(Word(hd_p->turfio.l3Type1Count,"l3cnt"));
  fields.push_back(Word(hd_p->turfio.trigTime,"trigtime"));
  fields.push_back(Word(hd_p->turfio.ppsNum,"pps"));
  fields.push_back(Word(hd_p->turfio.c3poNum,"c3po"));
  fields.push_back(Word(hd_p->turfio.deadTime,"deadtime"));
  fields.push_back(Word(hd_p->turfio.l3TrigPattern,"l3trigpat"));
  fields.push_back(Word(hd_p->turfio.l3TrigPatternH,"l3trigpath"));

  return;
}

//Event header parser
void Packet::parseCmd(){
  // Set DB table name, create pointer to data structure and check type again
  table="cmd";
  //  printf(" cmd packet.\n") ;
  //cerr<<"nData="<<nData<<" CommandEcho_t size="<<sizeof(CommandEcho_t)<<endl;
  if(nData!=sizeof(CommandEcho_t)) throw Error::ParseError();
  CommandEcho_t *cmd_p=(CommandEcho_t*)pckStruct_p;
  //cerr<<"cmd_p->gHdr.code="<<cmd_p->gHdr.code<<" PACKET_CMD_ECHO="<<PACKET_CMD_ECHO<<endl;
  if((cmd_p->gHdr.code&0xffff)!=PACKET_CMD_ECHO) throw Error::ParseError();

  // Store values
  fields.reserve(7);
  fields.push_back(Word(cmd_p->unixTime,"time"));
  fields.push_back(Word(cmd_p->goodFlag,"flag"));
  fields.push_back(Word(cmd_p->numCmdBytes,"bytes"));
  ostringstream cmd;
  cmd << "{";
  for(int b=0;b<cmd_p->numCmdBytes;++b){
    cmd << (unsigned int)cmd_p->cmd[b];
    if(b<cmd_p->numCmdBytes-1) cmd << ",";
    else cmd << "}";
  }
  fields.push_back(Word(cmd.str().c_str(),"cmd"));

  return;
}

// SURF housekeeping parser
void Packet::parseSurfHk(){
  // Set DB table name, create pointer to data structure and check type again
  table="hk_surf";
  //  printf(" hk_surf packet.\n") ;
  if(nData!=sizeof(FullSurfHkStruct_t)) throw Error::ParseError();
  FullSurfHkStruct_t *surf_p=(FullSurfHkStruct_t*)pckStruct_p;
  if((surf_p->gHdr.code&0xffff)!=PACKET_SURF_HK) throw Error::ParseError();

  // Store values
  fields.reserve(14);
  fields.push_back(Word(surf_p->unixTime,"time"));
  fields.push_back(Word(surf_p->unixTimeUs,"us"));
  fields.push_back(Word(surf_p->globalThreshold,"global"));
  fields.push_back(Word(surf_p->errorFlag,"error"));
  //*  fields.push_back(Word(surf_p->scalerGoal,"scalergoal"));
  ostringstream scalergoals,upper,surfmask,scaler,thresh,threshset,rfp,l1scaler;//peng
  scalergoals << "{";
  upper << "{";
  surfmask << "{";
  scaler << "{";
  thresh << "{";
  threshset << "{";
  rfp << "{";
  l1scaler << "{";//peng
  for(int i=0;i<NUM_ANTENNA_RINGS;++i){//peng
    scalergoals << surf_p->scalerGoals[i];
    if (i<NUM_ANTENNA_RINGS-1) {//peng
      scalergoals << "," ;
    }
    else {
      scalergoals << "}" ;
    }
      
  }
  for(int i=0;i<ACTIVE_SURFS;++i){
    upper << surf_p->upperWords[i];
    surfmask << surf_p->surfTrigBandMask[i];  // LSB logic, check...
    rfp << "{";
    for(int r=0;r<RFCHAN_PER_SURF;++r){
      rfp << surf_p->rfPower[i][r];
      if(r<RFCHAN_PER_SURF-1) rfp << ",";
      else rfp << "}";
    }
    if(i<ACTIVE_SURFS-1){
      upper << ",";
      surfmask << ",";
      rfp << ",";
    }else{ 
       upper << "}";
       surfmask << "}";
       rfp << "}";
    }
  }
  for(int i=0;i<ACTIVE_SURFS;++i){
    scaler << "{";
    thresh << "{";
    threshset << "{";
    for(int s=0;s<SCALERS_PER_SURF;++s){
      scaler << surf_p->scaler[i][s];
      thresh << surf_p->threshold[i][s];
      threshset << surf_p->setThreshold[i][s];
      if(s<SCALERS_PER_SURF-1){
	scaler << ",";
	thresh << ",";
	threshset << ",";
      }else{
	scaler << "}";
	thresh << "}";
	threshset << "}";
      }
    }
    if(i<ACTIVE_SURFS-1){
      scaler << ",";
      thresh << ",";
      threshset << ",";
    }else{ 
       scaler << "}";
       thresh << "}";
       threshset << "}";
    }
  }
/////////////peng///////////////
  for(int i=0;i<ACTIVE_SURFS;++i){
    l1scaler << "{";
    for(int s=0;s<L1S_PER_SURF;++s){
      l1scaler << surf_p->l1Scaler[i][s];
      if(s<L1S_PER_SURF-1){
	l1scaler << ",";
      }else{
	l1scaler << "}";
      }
    }
    if(i<ACTIVE_SURFS-1){
      l1scaler << ",";
    }else{ 
       l1scaler << "}";
    }
  }
/////////////peng///////////////
  fields.push_back(Word(scalergoals.str().c_str(),"scalergoals"));
  fields.push_back(Word(upper.str().c_str(),"upper"));
  fields.push_back(Word(scaler.str().c_str(),"scaler"));
  fields.push_back(Word(thresh.str().c_str(),"thresh"));
  fields.push_back(Word(threshset.str().c_str(),"threshset"));
  fields.push_back(Word(rfp.str().c_str(),"rfpow"));
  fields.push_back(Word(surfmask.str().c_str(),"surfmask"));
  fields.push_back(Word(l1scaler.str().c_str(),"l1scaler"));//peng

  return;
}


// TURF rate parser
void Packet::parseTurf(){
  // Set DB table name, create pointer to data structure and check type again
  table="turf";
  //  printf(" turf packet.\n") ;
  if(nData!=sizeof(TurfRateStruct_t)) throw Error::ParseError();
  TurfRateStruct_t *turf_p=(TurfRateStruct_t*)pckStruct_p;
  if((turf_p->gHdr.code&0xffff)!=PACKET_TURF_RATE) throw Error::ParseError();

  // Store values
  fields.reserve(7);
  fields.push_back(Word(turf_p->unixTime,"time"));
  fields.push_back(Word(turf_p->deadTime,"deadtime"));
  fields.push_back(Word(turf_p->phiTrigMask,"phitrigmask"));
  // fields.push_back(Word(turf_p->phiTrigMaskH,"phitrigmaskh"));//peng, removed 11/22/2016
  fields.push_back(Word(turf_p->l2TrigMask,"l2trigmask"));//peng
  // fields.push_back(Word(turf_p->l1TrigMaskH,"l1trigmaskh"));//peng, removed 11/22/2016
  ostringstream l2;
  l2 << "{";
  for(int j=0;j<PHI_SECTORS;++j){
    l2 << (unsigned short)(turf_p->l2Rates[j]);
    if(j<PHI_SECTORS-1) l2 << ",";
    else l2 << "}";
  }
  fields.push_back(Word(l2.str().c_str(),"l2"));
// //peng
//   ostringstream l1H;
//   l1H << "{";
//   for(int j=0;j<PHI_SECTORS;++j){
//     l1H << (unsigned short)(turf_p->l1Rates[j][1]);
//     if(j<PHI_SECTORS-1) l1H << ",";
//     else l1H << "}";
//   }
//   fields.push_back(Word(l1H.str().c_str(),"l1h"));

  ostringstream l3;
  l3 << "{";
  for(int j=0;j<PHI_SECTORS;++j){
    l3 << (unsigned short)(turf_p->l3Rates[j]);
    if(j<PHI_SECTORS-1) l3 << ",";
    else l3 << "}";
  }
  fields.push_back(Word(l3.str().c_str(),"l3"));
// //peng
//   ostringstream l3H;
//   l3H << "{";
//   for(int j=0;j<PHI_SECTORS;++j){
//     l3H << (unsigned short)(turf_p->l3Rates[j][1]);
//     if(j<PHI_SECTORS-1) l3H << ",";
//     else l3H << "}";
//   }
//   fields.push_back(Word(l3H.str().c_str(),"l3h"));

  ostringstream l3gated;
  l3gated << "{";
  for(int j=0;j<PHI_SECTORS;++j){
    l3gated << (unsigned short)(turf_p->l3RatesGated[j]);
    if(j<PHI_SECTORS-1) l3gated << ",";
    else l3gated << "}";
  }
  fields.push_back(Word(l3gated.str().c_str(),"l3gated"));

  //  printf(" %s\n", fields) ;

  return;
}

// RTLSDR POW SPECTRUM parser
void Packet::parseRtlsdr(){
  table="rtlsdr";
  //  printf(" rtlsdr packet.\n") ;
  if(nData!=sizeof(RtlSdrPowerSpectraStruct_t)) throw Error::ParseError();
  RtlSdrPowerSpectraStruct_t *rtlsdr_p=(RtlSdrPowerSpectraStruct_t*)pckStruct_p;
  if((rtlsdr_p->gHdr.code&0xffff)!=PACKET_RTLSDR_POW_SPEC) throw Error::ParseError();

  // Store values
  fields.reserve(8);
  fields.push_back(Word(rtlsdr_p->nFreq,"nfreq"));
  fields.push_back(Word(rtlsdr_p->startFreq,"startfreq"));
  fields.push_back(Word(rtlsdr_p->freqStep,"freqstep"));
  fields.push_back(Word(rtlsdr_p->unixTimeStart,"unixtimestart"));
  fields.push_back(Word(rtlsdr_p->scanTime,"scantime"));
  fields.push_back(Word(rtlsdr_p->gain,"gain"));
  fields.push_back(Word(rtlsdr_p->rtlNum,"rtlnum"));
  ostringstream spectrum;
  spectrum << "{";
  for(int j=0;j<RTLSDR_MAX_SPECTRUM_BINS;++j){
    spectrum << (char)(rtlsdr_p->spectrum[j]);
    if(j<RTLSDR_MAX_SPECTRUM_BINS-1) spectrum << ",";
    else spectrum << "}";
  }
  fields.push_back(Word(spectrum.str().c_str(),"spectrum"));
  return;
}

// TUFF STATUS parser
void Packet::parseTuffStatus(){
  table="tuffstatus";
  //  printf(" tuff status packet.\n") ;
  if(nData!=sizeof(TuffNotchStatus_t)) throw Error::ParseError();
  TuffNotchStatus_t *tuffstatus_p=(TuffNotchStatus_t*)pckStruct_p;
  if((tuffstatus_p->gHdr.code&0xffff)!=PACKET_TUFF_STATUS) throw Error::ParseError();

  // Store values
  fields.reserve(5);
  fields.push_back(Word(tuffstatus_p->unixTime,"time"));
  fields.push_back(Word(tuffstatus_p->notchSetTime,"notchsettime"));
  ostringstream startsectors;
  startsectors << "{";
  for(int j=0;j<NUM_TUFF_NOTCHES;++j){
    startsectors << (char)(tuffstatus_p->startSectors[j]);
    if(j<NUM_TUFF_NOTCHES-1) startsectors << ",";
    else startsectors << "}";
  }
  fields.push_back(Word(startsectors.str().c_str(),"startsectors"));
  ostringstream endsectors;
  endsectors << "{";
  for(int j=0;j<NUM_TUFF_NOTCHES;++j){
    endsectors << (char)(tuffstatus_p->endSectors[j]);
    if(j<NUM_TUFF_NOTCHES-1) endsectors << ",";
    else endsectors << "}";
  }
  fields.push_back(Word(endsectors.str().c_str(),"endsectors"));
  ostringstream temperatures;
  temperatures << "{";
  for(int j=0;j<NUM_RFCM;++j){
    temperatures << (char)(tuffstatus_p->temperatures[j]);
    if(j<NUM_RFCM-1) temperatures << ",";
    else temperatures << "}";
  }
  fields.push_back(Word(temperatures.str().c_str(),"temperatures"));
  return;
}

// TUFF RAW CMD parser
void Packet::parseTuffCmd(){
  table="tuffcmd";
  //  printf(" tuff cmd packet.\n") ;
  if(nData!=sizeof(TuffRawCmd_t)) throw Error::ParseError();
  TuffRawCmd_t *tuffcmd_p=(TuffRawCmd_t*)pckStruct_p;
  if((tuffcmd_p->gHdr.code&0xffff)!=PACKET_TUFF_RAW_CMD) throw Error::ParseError();

  // Store values
  fields.reserve(5);
  fields.push_back(Word(tuffcmd_p->requestedTime,"requestedtime"));
  fields.push_back(Word(tuffcmd_p->enactedTime,"enactedtime"));
  fields.push_back(Word(tuffcmd_p->cmd,"short"));
  fields.push_back(Word(tuffcmd_p->irfcm,"irfcm"));
  fields.push_back(Word(tuffcmd_p->tuffStack,"tuffstack"));
  return;
}

// CPU monotor parser
void Packet::parseMonitor(){
  // Set DB table name, create pointer to data structure and check type again
  table="mon";
  //  printf(" mon packet.\n") ;
  if(nData!=sizeof(MonitorStruct_t)) throw Error::ParseError();
  MonitorStruct_t *mon_p=(MonitorStruct_t*)pckStruct_p;
  if((mon_p->gHdr.code&0xffff)!=PACKET_MONITOR) throw Error::ParseError();

  // Store values
  fields.reserve(17);
  fields.push_back(Word(mon_p->unixTime,"time"));
  ostringstream disk;
  disk << "{";
  for(int i=0;i<8;++i){
    disk << (mon_p->diskInfo.diskSpace[i]); 
    if(i<7) disk << ",";
    else disk << "}";
  }
  fields.push_back(Word(disk.str().c_str(),"disk"));
  //*  fields.push_back(Word(mon_p->diskInfo.bladeLabel,"blade"));
  //*  fields.push_back(Word(mon_p->diskInfo.usbIntLabel,"usbint"));
  //*  fields.push_back(Word(mon_p->diskInfo.usbIntLabel,"usbext"));

  //*  fields.push_back(Word(mon_p->queueInfo.cmdLinksLOS,"linkcmdlos"));
  //*  fields.push_back(Word(mon_p->queueInfo.cmdLinksSIP,"linkcmdsip"));
  //*  fields.push_back(Word(mon_p->queueInfo.gpsLinks,"linkgps"));
  //*  fields.push_back(Word(mon_p->queueInfo.hkLinks,"linkhk"));
  //*  fields.push_back(Word(mon_p->queueInfo.monitorLinks,"linkmon"));
  //*  fields.push_back(Word(mon_p->queueInfo.headLinks,"linkhd"));
  //*  fields.push_back(Word(mon_p->queueInfo.surfHkLinks,"linksurf"));
  //*  fields.push_back(Word(mon_p->queueInfo.turfHkLinks,"linkturf"));
  //*  fields.push_back(Word(mon_p->queueInfo.pedestalLinks,"linkped"));
  
  ostringstream linkev;
  linkev << "{";
  for(int i=0;i<NUM_PRIORITIES;++i){
    linkev << mon_p->queueInfo.eventLinks[i];
    if(i<NUM_PRIORITIES-1) linkev << ",";
    else linkev << "}";
  }
  fields.push_back(Word(linkev.str().c_str(),"linkev"));
  return;
}

//Waveform packet parser
void Packet::parseWv(){
  // Set DB table name, create pointer to data structure and check type again
  table="wv";
  //  printf(" wv packet.\n") ;
  if(nData!=sizeof(RawWaveformPacket_t)) throw Error::ParseError();
  RawWaveformPacket_t *wv_p=(RawWaveformPacket_t*)pckStruct_p;
  if((wv_p->gHdr.code&0xffff)!=PACKET_WV   &&
     (wv_p->gHdr.code&0xffff)!=PACKET_SURF &&
     (wv_p->gHdr.code&0xffff)!=PACKET_ENC_SURF) throw Error::ParseError();
  // Prepare temporay storage of waveform packet, since this packet info is not
  // permanent in the scope of main program
  // If fake_pp is set at this point, this packet came as zipped, so we already stored
  // it in a temporrary location
  if(!fake_pp){
    fake_pp=(void*)(new unsigned char[nData]);
    memcpy(fake_pp,wv_p,nData);
  }
    
  // Store values
  fields.reserve(13);
  fields.push_back(Word(wv_p->eventNumber,"evnum"));
  fields.push_back(Word(wv_p->waveform.header.chanId,"id"));
  fields.push_back(Word(wv_p->waveform.header.chipIdFlag&0x3,"chip"));
  fields.push_back(Word((wv_p->waveform.header.chipIdFlag&0x4)>>2,"rcobit"));
  fields.push_back(Word((wv_p->waveform.header.chipIdFlag&0x8)>>3,"hbwrap"));
  fields.push_back(Word(wv_p->waveform.header.firstHitbus,"hbstart"));
  fields.push_back(Word(wv_p->waveform.header.lastHitbus+(wv_p->waveform.header.chipIdFlag>>4),"hbend"));
  fields.push_back(Word(0,"peds"));  // 0 will mark it as uncalibrated
  ostringstream data;
  data << "{";
  for(int s=0;s<MAX_NUMBER_SAMPLES;++s){
    data << (wv_p->waveform.data[s]&0xfff);
    if(s<MAX_NUMBER_SAMPLES-1) data << ",";
    else data << "}";
  }
    fields.push_back(Word(data.str().c_str(),"raw"));
  //   fields.push_back(Word(data.str().c_str(),"cal"));//amirs addition

  return;
}

//Waveform packet parser
void Packet::parsePsWv(){
  // Set DB table name, create pointer to data structure and check type again
  table="wv";
  //  printf(" wv(pswv) packet.\n") ;
  if(nData!=sizeof(PedSubbedWaveformPacket_t)) throw Error::ParseError();
  PedSubbedWaveformPacket_t *wv_p=(PedSubbedWaveformPacket_t*)pckStruct_p;
  if((wv_p->gHdr.code&0xffff)!=PACKET_PEDSUB_WV     &&
     (wv_p->gHdr.code&0xffff)!=PACKET_ENC_WV_PEDSUB &&
     (wv_p->gHdr.code&0xffff)!=PACKET_PEDSUB_SURF   &&
     (wv_p->gHdr.code&0xffff)!=PACKET_ENC_SURF_PEDSUB) throw Error::ParseError();
  // Prepare temporay storage of waveform packet, since this packet info is not
  // permanent in the scope of main program
  // If fake_pp is set at this point, this packet came as zipped, so we already stored
  // it in a temporrary location
  if(!fake_pp){
    fake_pp=(void*)(new unsigned char[nData]);
    memcpy(fake_pp,wv_p,nData);
  }
  //cerr<<" Id = "<<(int)wv_p->waveform.header.chanId<<endl;  
  // Store values
  fields.reserve(13);
  fields.push_back(Word(wv_p->eventNumber,"evnum"));
  fields.push_back(Word(wv_p->waveform.header.chanId,"id"));
  fields.push_back(Word(wv_p->waveform.header.chipIdFlag&0x3,"chip"));
  fields.push_back(Word((wv_p->waveform.header.chipIdFlag&0x8)>>3,"hbwrap"));
  fields.push_back(Word(wv_p->waveform.header.firstHitbus,"hbstart"));
  fields.push_back(Word(wv_p->waveform.header.lastHitbus+(wv_p->waveform.header.chipIdFlag>>4),"hbend"));
  fields.push_back(Word(wv_p->whichPeds,"peds"));  
  //// amir's calibration code is here////////////////////////
  int hbwrap_1 =int((wv_p->waveform.header.chipIdFlag&0x8)>>3);
  int hbstart_1=int(wv_p->waveform.header.firstHitbus);
  int hbend_1=int(wv_p->waveform.header.lastHitbus+(wv_p->waveform.header.chipIdFlag>>4));
  //calib_out<<"hbwrap_1= "<<hbwrap_1<<' '<<"hbstart_1= "<<hbstart_1<<' '<<"hbend_1= "<<hbend_1<<std::endl;//debug mode 
  double cal_data[260];
  double rot_cal_data[260];
  double raw_data[260];
  int ir=0;
  //calibrate voltages
  // calib_out<<"raw data = "<<std::endl;// debug mode
  for(int s=0;s<MAX_NUMBER_SAMPLES;++s){
  raw_data[s]=(wv_p->waveform.data[s]);
  // calib_out<<wv_p->waveform.data[s]<<' ';
   if(raw_data[s]==0){
   cal_data[s]=0;
  	}
   else
  cal_data[s]=raw_data[s]*2*1.17;
  	}
  // calib_out<<std::endl;//debug mode 
   // rotation
  if(hbwrap_1==1){
  	for(int i=(hbstart_1+1);i<=hbend_1;i++){
		rot_cal_data[ir]=cal_data[i];
		ir++;
  		}
  	}
  else
  	{
  	for(int i=(hbend_1+1);i<260;i++){
		rot_cal_data[ir]=cal_data[i];
		ir++;
  		}
	for(int i=1;i<=hbstart_1;i++){
		rot_cal_data[ir]=cal_data[i];
		ir++;
		}
  	}
  for(int i=ir;i<260;i++){
  	rot_cal_data[i]=0;
  	}
  /*
  calib_out<<"rot_cal_data = "<<std::endl;
  for(int i=0;i<MAX_NUMBER_SAMPLES;i++){
	calib_out<<rot_cal_data[i]<<' ';
	}
  calib_out<<std::endl;
*/
  /////////////////////////////////////////////////////////////
  ostringstream data;
  data << "{";
  for(int s=0;s<MAX_NUMBER_SAMPLES;++s){
    // data << wv_p->waveform.data[s];
    // amirs addition of calibration code ////////////////
    data << rot_cal_data[s];
    /////////////////////////////////////////////////////
    if(s<MAX_NUMBER_SAMPLES-1) data << ",";
    else data << "}";
  }
  //   fields.push_back(Word(data.str().c_str(),"raw"));
      fields.push_back(Word(data.str().c_str(),"cal"));//amir's addition
  
  return;
}

//Wakeup packet parsing
void Packet::parseWakeupLOS(){
  table="wakeup";
  fields.push_back(Word(0,"type"));
}
void Packet::parseWakeupHiRate(){
  table="wakeup";
  fields.push_back(Word(1,"type"));
}
void Packet::parseWakeupCOMM1(){
  table="wakeup";
  fields.push_back(Word(2,"type"));
}
void Packet::parseWakeupCOMM2(){
  table="wakeup";
  fields.push_back(Word(3,"type"));
}

// Slow data parsing
void Packet::parseSlowFull(){
  // Set DB table name, create pointer to data structure and check type again
  table="slow";
  //  printf(" slow packet.\n") ;
  if(nData!=sizeof(SlowRateFull_t)) throw Error::ParseError();
  SlowRateFull_t *slow_p=(SlowRateFull_t*)pckStruct_p;
  if((slow_p->gHdr.code&0xffff)!=PACKET_SLOW_FULL) throw Error::ParseError();

  // Store values
  fields.reserve(10);
  fields.push_back(Word(slow_p->unixTime,"time"));
  fields.push_back(Word(slow_p->rf.eventNumber,"evnum"));
  fields.push_back(Word(slow_p->hk.latitude,"latitude"));
  fields.push_back(Word(slow_p->hk.longitude,"longitude"));
  fields.push_back(Word(slow_p->hk.altitude+65536,"altitude"));
  fields.push_back(Word((int)slow_p->rf.eventRate1Min,"rate1"));
  fields.push_back(Word((int)slow_p->rf.eventRate10Min,"rate10"));
/*
  fields.push_back(Word((int)slow_p->rf.eventRate1Min/(float)8.,"rate1"));
  fields.push_back(Word((int)slow_p->rf.eventRate10Min/(float)8.,"rate10"));
*/
  ostringstream avgscaler,avgrfp;
  //ostringstream avgscaler,rmsscaler,avgrfp;
  avgscaler << "{";
//  rmsscaler << "{";
  avgrfp << "{";
  for(int i=0;i<ACTIVE_SURFS;++i){
    avgrfp << "{";
    for(int r=0;r<RFCHAN_PER_SURF;++r){
      avgrfp << (unsigned short)(slow_p->rf.rfPwrAvg[i][r]*128);
      if(r<RFCHAN_PER_SURF-1) avgrfp << ",";
      else avgrfp << "}";
    }
    if(i<ACTIVE_SURFS-1) avgrfp << ",";
    else avgrfp << "}";
  }
  for(int i=0;i<TRIGGER_SURFS;++i){
    avgscaler << "{";
//    rmsscaler << "{";
//    for(int s=0;s<ANTS_PER_SURF;++s){
    //for(int s=0;s<TRIGGERS_PER_SURF;++s){
    //[TRIGGER_SURFS][SCALERS_PER_SURF]
    for(int s=0;s<SCALERS_PER_SURF;++s){
      avgscaler << (unsigned short)(slow_p->rf.avgScalerRates[i][s]*4); 
      //avgscaler << (unsigned short)(slow_p->rf.avgScalerRates[i][s]*128); 
//      rmsscaler << (unsigned short)(slow_p->rf.rmsScalerRates[i][s]);
      //if(s<TRIGGERS_PER_SURF-1){
      if(s<SCALERS_PER_SURF-1){
	avgscaler << ",";
//	rmsscaler << ",";
      }else{
	avgscaler << "}";
//	rmsscaler << "}";
      }
    }
    if(i<TRIGGER_SURFS-1){
      avgscaler << ",";
//      rmsscaler << ",";
    }else{ 
      avgscaler << "}";
//      rmsscaler << "}";
    }
  }
  fields.push_back(Word(avgscaler.str().c_str(),"avgscaler"));
//  fields.push_back(Word(rmsscaler.str().c_str(),"rmsscaler"));
  fields.push_back(Word(avgrfp.str().c_str(),"avgrfpow"));
/*
  ostringstream avgl1,avgl2,avgl3;
  avgl1 << "{";
  avgl2 << "{";
  avgl3 << "{";
  for(int i=0;i<ACTIVE_SURFS;++i){
    avgl1 << (unsigned short)(slow_p->rf.avgL1Rates[i]);
    if(i<ACTIVE_SURFS-1) avgl1 << ",";
    else avgl1 << "}";
  }
//   avgl2 << "{";
//   for(int j=0;j<PHI_SECTORS;++j){
//     avgl2 << (unsigned short)(slow_p->rf.avgUpperL2Rates[j]);
//     if(j<PHI_SECTORS-1) avgl2 << ",";
//     else avgl2 << "}";
//   }
//   avgl2 << ",{";
//   for(int j=0;j<PHI_SECTORS;++j){
//     avgl2 << (unsigned short)(slow_p->rf.avgLowerL2Rates[j]);
//     if(j<PHI_SECTORS-1) avgl2 << ",";
//     else avgl2 << "}";
//   }
//   avgl2 << "}";
// avgl2 now is one dimentional (average of upper/lower).
  for(int j=0;j<PHI_SECTORS;++j){
    avgl2 << (unsigned short)(slow_p->rf.avgL2Rates[j]);
    if(j<PHI_SECTORS-1) avgl2 << ",";
    else avgl2 << "}";
  }
  for(int j=0;j<PHI_SECTORS;++j){
    avgl3 << (unsigned short)(slow_p->rf.avgL3Rates[j]);
    if(j<PHI_SECTORS-1) avgl3 << ",";
    else avgl3 << "}";
  }
  fields.push_back(Word(avgl1.str().c_str(),"avgl1"));
  fields.push_back(Word(avgl2.str().c_str(),"avgl2"));
  fields.push_back(Word(avgl3.str().c_str(),"avgl3"));
*/

  ostringstream temp,power;
  temp << "{" << (short)(slow_p->hk.temps[0])*4 << ",";
  for(int j=1;j<8;++j){
    temp << (((unsigned short)((unsigned char)slow_p->hk.temps[j]))*16);
    if(j<7) temp << ",";
    else temp << "}";
  }
  power << "{";
  for(int j=0;j<4;++j){
    power << (((unsigned short)((unsigned char)slow_p->hk.powers[j]))*16);
    if(j<3) power << ",";
    else power << "}";
  }
  fields.push_back(Word(temp.str().c_str(),"tempraw"));
  fields.push_back(Word(power.str().c_str(),"powerraw"));

  return;
}

void Packet::parsePed(){
  // Set DB table name, create pointer to data structure and check type again
  table="wv_ped";
  //  printf(" wv_ped packet.\n") ;
  if(nData!=sizeof(FullLabChipPedStruct_t)) throw Error::ParseError();
  FullLabChipPedStruct_t *ped_p=(FullLabChipPedStruct_t*)pckStruct_p;
  long nChan=((long)ped_p->gHdr.code)>>16; // Channel to parse stored in upper two bytes
  ped_p->gHdr.code=(PacketCode_t)(((long)ped_p->gHdr.code)&0xffff);  // Clear channel number info
  if((ped_p->gHdr.code&0xffff)!=PACKET_LAB_PED) throw Error::ParseError();

  // Store values
  fields.reserve(10);
  fields.push_back(Word(ped_p->unixTimeStart,"start"));
  fields.push_back(Word(ped_p->unixTimeEnd,"time"));
  fields.push_back(Word(ped_p->pedChan[nChan].chanId,"id"));
  fields.push_back(Word(ped_p->pedChan[nChan].chipId,"chip"));
  fields.push_back(Word(ped_p->pedChan[nChan].chipEntries,"entries"));
  ostringstream pedMean;
  ostringstream pedRMS;
  pedMean << "{";
  pedRMS << "{";
  for(int s=0;s<MAX_NUMBER_SAMPLES;++s){
    pedMean << (ped_p->pedChan[nChan].pedMean[s]/10.);  // Values sent x10
    pedRMS << (ped_p->pedChan[nChan].pedRMS[s]/10.); // Values sent x10
    if(s<MAX_NUMBER_SAMPLES-1){
      pedMean << ",";
      pedRMS << ",";
    }else{
      pedMean << "}";
      pedRMS << "}";
    }
  }
  fields.push_back(Word(pedMean.str().c_str(),"ped"));
  fields.push_back(Word(pedRMS.str().c_str(),"rms"));

  return;
}

void Packet::parseFile(){
  // Set DB table name, create pointer to data structure and check type again
  table="file";
  ZippedFile_t *zip_p=(ZippedFile_t*)pckStruct_p;
  if((zip_p->gHdr.code&0xffff)!=PACKET_ZIPPED_FILE) throw Error::ParseError();

  // Store values
  fields.reserve(7);
  fields.push_back(Word(zip_p->unixTime,"time"));
  fields.push_back(Word(zip_p->filename,"filename"));
  fields.push_back(Word(zip_p->numUncompressedBytes,"length"));
  unsigned long unzippedBytes=zip_p->numUncompressedBytes;
  int zippedBytes=zip_p->gHdr.numBytes-sizeof(ZippedFile_t);
  char *text=new char[unzippedBytes+1];
  //cerr<<"unzippedBytes: "<<unzippedBytes<<endl;
  //cerr<<"zippedBytes: "<<zippedBytes<<endl;
  int retVal=uncompress((Bytef*)text,(uLongf*)&unzippedBytes,(const Bytef*)((const char*)zip_p+sizeof(ZippedFile_t)),zippedBytes);
  //cerr<<"retVal: "<<dec<<retVal<<endl;
  if(retVal!=Z_OK || unzippedBytes!=zip_p->numUncompressedBytes){
    delete[] text;
    throw Error::UnzipError((unsigned char *)pckStruct_p,nData);
  }
  char *sqlText=new char[unzippedBytes*2];
  PQescapeString(sqlText,text,unzippedBytes);

  fields.push_back(Word(sqlText,"content"));
  delete[] text;
  delete[] sqlText;

  return;
}

// Not implemented
void Packet::parseRun(){
  // Set DB table name, create pointer to data structure and check type again
  table="run";
  //  printf(" run packet.\n") ;
  RunStart_t *run_p=(RunStart_t*)pckStruct_p;
  if((run_p->gHdr.code&0xffff)!=PACKET_RUN_START) throw Error::ParseError();

  // Store values
  fields.reserve(6);
  fields.push_back(Word(run_p->unixTime,"time"));
  fields.push_back(Word(run_p->eventNumber,"evnum"));
  fields.push_back(Word(run_p->runNumber,"run"));

  return;
}

// Not implemented
void Packet::parseOtherMonitor(){
   // Set DB table name, create pointer to data structure and check type again
  table="other";
  OtherMonitorStruct_t *mon_p=(OtherMonitorStruct_t*)pckStruct_p;
  //  printf(" other monitor packet.\n") ;
  if((mon_p->gHdr.code&0xffff)!=PACKET_OTHER_MONITOR) throw Error::ParseError();

  // Store values
  fields.reserve(15);
  fields.push_back(Word(mon_p->unixTime,"time"));
  fields.push_back(Word(mon_p->ramDiskInodes,"raminodes"));
  fields.push_back(Word(mon_p->runStartTime,"runtime"));
  fields.push_back(Word(mon_p->runStartEventNumber,"runevnum"));
  fields.push_back(Word(mon_p->runNumber,"runnum"));
  fields.push_back(Word(mon_p->dirFiles[0],"filesacqd"));
  fields.push_back(Word(mon_p->dirFiles[1],"fileseventd"));
  fields.push_back(Word(mon_p->dirFiles[2],"filesprior"));
  fields.push_back(Word(mon_p->dirLinks[0],"lnacqd"));
  fields.push_back(Word(mon_p->dirLinks[1],"lneventd"));
  fields.push_back(Word(mon_p->dirLinks[2],"lnprior"));
  //*  fields.push_back(Word(mon_p->otherFlag,"other"));

  return;
}
