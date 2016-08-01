/* Class declarations for unpackd objects */

#include <string>
#include <vector>
#include <ext/hash_map>
#include "word.h"
extern "C"{
#include <includes/anitaStructures.h>
#include "../lib/anitaGSE.h"
}
using std::string;
using namespace __gnu_cxx;

#define INFOMAX 2048  // Need this to be big, since error dump can get large

//Define error exceptions
namespace Error{
  struct ParseError{};

  struct BaseError{
    char message[128];
    char action[128];
    char info[INFOMAX];
    BaseError(){message[0]='\0';info[0]='\0';action[0]='\0';}
  };

  struct NullStructure:BaseError{
    NullStructure(){sprintf(message,"Encountered NULL packet structure pointer.");}
  };

  struct EmptyLine:BaseError{
    EmptyLine(){sprintf(message,"Encountered empty line in a data stream.");}
  };

  struct UnzipError:BaseError{
    UnzipError(unsigned char *data,int n){
      sprintf(message,"Unzipping failed!");
      sprintf(action,"Skipping!");
      int c=0;
      for(int i=0;i<n;++i){
	c+=sprintf(&info[c],"%02x ",data[i]);
	if(c>=INFOMAX-5){
	  sprintf(&info[c],"...");
	  break;
	}
      }
    }
  };

  struct UnmatchedPacketType:BaseError{
    UnmatchedPacketType(unsigned char *data,int n,int code){
      sprintf(message,"Input packet unmatched, code %x",code);
      sprintf(action,"Skipping!");
      int c=0;
      for(int i=0;i<n;++i){
	c+=sprintf(&info[c],"%02x ",data[i]);
	if(c>=INFOMAX-5){
	  sprintf(&info[c],"...");
	  break;
	}
      }
    }
  };
  
  struct UnknownEncodingType:BaseError{
    UnknownEncodingType(unsigned char *data,int n,long evnum,int code){
      sprintf(message,"Unknown encoding type encountered, event %ld, code %x",evnum,code);//amir's change
      sprintf(action,"Skipping waveform!");
      int c=0;
      for(int i=0;i<n;++i){
	c+=sprintf(&info[c],"%02x ",data[i]);
	if(c>=INFOMAX-5){
	  sprintf(&info[c],"...");
	  break;
	}
      }
    }
  };
  
  struct DecodingError:BaseError{
    DecodingError(unsigned char *data,int n,long evnum,int code){
      // sprintf(message,"Waveform decoding error, event %ld, error %d",code);original code
      sprintf(message,"Waveform decoding error, event %ld, error %d",evnum,code);//amir's change
      sprintf(action,"Skipping waveform!");
      int c=0;
      for(int i=0;i<n;++i){
	c+=sprintf(&info[c],"%02x ",data[i]);
	if(c>=INFOMAX-5){
	  sprintf(&info[c],"...");
	  break;
	}
      }
    }
  };
  
  struct MalformatedPacket:BaseError{
    MalformatedPacket(unsigned char *data,int n){
      sprintf(message,"Malformated packet.");
      sprintf(action,"Skipping!");
      int c=0;
      for(int i=0;i<n;++i){
	c+=sprintf(&info[c],"%02x ",data[i]);
	if(c>=INFOMAX-5){
	  sprintf(&info[c],"...");
	  break;
	}
      }
    }
  };

  struct FailedInitialization:BaseError{
    FailedInitialization(unsigned char *data,int n){
      sprintf(message,"Failed initialization.");
      sprintf(action,"Skipping!");
      int c=0;
      for(int i=0;i<n;++i){
	c+=sprintf(&info[c],"%02x ",data[i]);
	if(c>=INFOMAX-10){
	  sprintf(&info[c],"...");
	  break;
	}
      }
    }
  };

  struct CRCError:BaseError{
    CRCError(unsigned char *data,int n){	     
      sprintf(message,"Failed CRC checksum.");
      sprintf(action,"Skipping!");
      int c=0;
      for(int i=0;i<n;++i){
	c+=sprintf(&info[c],"%02x ",data[i]);
	if(c>=INFOMAX-10){
	  sprintf(&info[c],"...");
	  break;
	}
      }
    }
  };

  struct DBError:BaseError{
    DBError(const char *str,int code){	     
      sprintf(message,"Failed to store packet into database. PQ error code %d.",code);
      sprintf(action,"Skipping!");
      sprintf(info,str);
    }
  };

}

// Root packet class, contains abstractions of packet data and DB storage routines
class Packet{
public:
  // Default constructor
  Packet():pckStruct_p(NULL),nData(0),fake_pp(NULL){}

  // Constructor
  Packet(unsigned char *data,int n);
  
  // Alternate constructor
  Packet(void *ptr,int n,DataType_t type,bool crcWrapper,int crcInternal=-1);

  // Copy constructor
  Packet(const Packet &old);
 

  // Destructor
  ~Packet();

  // Store packet into database
  void store(PGconn*);

  // Reseting
  void clear();

  // Return start of packet's data
  const void *GetPacketPointer(){return fake_pp?fake_pp:pckStruct_p;}

  // Return packet's data length
  const unsigned short getLength(){return nData;}

  // Create sql command
  string sqlCommand();

protected:
  // Check wrapper, returns status of wrapper crc
  bool checkWrapper(unsigned char *data,int n,DataType_t *type);

  // Parse packet and store fields
  void parsePacket(DataType_t type,bool crcWrapper,int crcInternal=-1);
  
  // Check internal checksum
  bool InternalCRC(unsigned char *ptr);

  // Pointer to beginning of data structure
  void *pckStruct_p;
  // Length of data structure in bytes
  unsigned short nData;
  // Data fields
  vector<Word> fields;
  // DB table, i.e. identifying the packet type 
  string table;

  // Parsing of different packet types
  void parseHd();
  void parseWv();
  void parsePsWv();
  void parseSurfHk();
  void parseTurf();
  void parseAdu5Pat(unsigned int gpstype);
  void parseAdu5Vtg(unsigned int gpstype);
  void parseAdu5Sat(unsigned int gpstype);
  void parseG12Pos();
  void parseG12Sat();
  void parseHkd();
  AnalogueCode_t  guessCode(SSHkDataStruct_t *hkPtr);
  void parseSSHkd();
  void parseCmd();
  void parseMonitor();
  void parseWakeupLOS();
  void parseWakeupHiRate();
  void parseWakeupCOMM1();
  void parseWakeupCOMM2();
  void parseSlowFull();
  void parsePed();
  void parseFile();
  void parseRun();
  void parseOtherMonitor();

 private:
  void *fake_pp;
};

// Wrapper class which handles multiple data packets per single transfer packet
class XferPacket:Packet{
  public:
  // Constructor
  XferPacket(unsigned char *data,int n);
 
  // Destructor
  ~XferPacket(){packets.clear();}

  // Store packets into database
  void store(PGconn*);

  // Reseting
  void clear(){packets.clear();}
  
  // Packets
  vector<Packet> packets;

 protected:
  bool WaveformCRC(unsigned char *ptr);
};
