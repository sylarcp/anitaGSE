/* Class declarations for unpackd objects */


#include <string>
#include <vector>
#include <ext/hash_map>
#include "word.h"
using std::string;
using namespace __gnu_cxx;

//Define error exceptions
namespace Error{
  struct BaseError{
    char message[128];
    char action[128];
    char info[1024];
    BaseError(){message[0]='\0';info[0]='\0';action[0]='\0';}
  };

  struct EmptyLine:BaseError{
    EmptyLine(){sprintf(message,"Encountered empty line in a data stream.");}
  };

  struct PthreadsError:BaseError{
    PthreadsError(unsigned char *data,int n){
      sprintf(message,"Failed to start thread!");
      sprintf(action,"Skipping!");
      for(int i=0;i<n;++i) sprintf(info,"%s %02x",info,data[i]);
    }
  };

  struct NotAnitaPacket:BaseError{
    NotAnitaPacket(unsigned char *data,int n){
      sprintf(message,"Encountered non-Anita packet in the data stream.");
      sprintf(action,"Skipping!");
      for(int i=0;i<n;++i) sprintf(info,"%s %02x",info,data[i]);
    }
  };

  struct UnmatchedPacketType:BaseError{
    UnmatchedPacketType(unsigned char *data,int n){
      sprintf(message,"Input packet unmatched.");
      sprintf(action,"Skipping!");
      for(int i=0;i<n;++i) sprintf(info,"%s %02x",info,data[i]);
    }
  };
  
  struct MalformatedPacket:BaseError{
    MalformatedPacket(unsigned char *data,int n){
      sprintf(message,"Malformated packet.");
      sprintf(action,"Skipping!");
      for(int i=0;i<n;++i) sprintf(info,"%s %02x",info,data[i]);
    }
  };

  struct EmptyPacket:BaseError{
    EmptyPacket(){
      sprintf(message,"Encountered empty Anita packet.");
      sprintf(action,"Skipping!");
    }
  };

  struct FailedInitialization:BaseError{
    FailedInitialization(unsigned char *data,int n){
      sprintf(message,"Failed initialization (likely parse error).");
      sprintf(action,"Skipping!");
      for(int i=0;i<n;++i) sprintf(info,"%s %02x",info,data[i]);
    }
  };

  struct CRCError:BaseError{
    CRCError(unsigned char *data,int n){	     
      sprintf(message,"Failed CRC checksum.");
      sprintf(action,"Skipping!");
      for(int i=0;i<n;++i) sprintf(info,"%s %02x",info,data[i]);
    }
  };
}

// Root packet class, contains abstractions of packet data and DB storage routines
class Packet{
public:
  // Constructor
  Packet(unsigned char *data,int n);
  
  // Destructor
  ~Packet(){fields.clear();}

  // Store packet into database, returns success flag
  bool store(PGconn*);

  // Reseting
  void clear(){fields.clear();table="";}
protected:
  // Data fields
  vector<Word> fields;
  // DB table, i.e. identifying the packet type 
  string table;
};

// Housekeeping1 packet class
class Hk1Packet:public Packet{
 public:
  // Constructor
  Hk1Packet(unsigned char *data,int n);

  // Destructor
  ~Hk1Packet(){};
};


// Housekeeping2 packet class
class Hk2Packet:public Packet{
 public:
  // Constructor
  Hk2Packet(unsigned char *data,int n);

  // Destructor
  ~Hk2Packet(){};
};

// Housekeeping4 packet class
class Hk4Packet:public Packet{
 public:
  // Constructor
  Hk4Packet(unsigned char *data,int n);

  // Destructor
  ~Hk4Packet(){};
};

// Command echo packet class
class CmdPacket:public Packet{
 public:
  // Constructor
  CmdPacket(unsigned char *data,int n);

  // Destructor
  ~CmdPacket(){};
};


// Header packet class
class HdPacket:public Packet{
 public:
  // Constructor
  HdPacket(unsigned char *data,int n);

  // Destructor
  ~HdPacket(){};
  
  // Return event number
  inline int getEvnum(){return fields.size()>=6?fields[5].getInt():-1;}

  // Return number of samples
  inline int getNsamp(){return fields.size()>=11?fields[10].getInt():-1;}
};

//Waveform transient packet class
class WvPacket:public Packet{
 public:
  // Constructor
  WvPacket(unsigned char *data,int n);

  // Destructor
  ~WvPacket(){wvdata.clear();}

  // Insert data we need from header packet
  void insertHeader(unsigned char *data,int n);

  // Insert data we need from transient packet
  void insertTransient(unsigned char *data,int n,bool validated=true);

  // Report if all transient packets are stored
  inline int done(){return Npck==wvdata.size();}

  // Reimplementation of DB storage function
  bool store(PGconn*);

  // Return event number
  inline int getEvnum(){return fields.size()>=6?fields[5].getInt():-1;}

 protected:
  // Number of transient packet to expect
  int Npck;

  // Number of samples per channel
  int Nsamp;

  // Hash_map of transient data
  hash_map<int,unsigned char*> wvdata;

 private:
  // Immutable number of samples per transient packet
  const static int Nsamppck=98;

  // Immutable number of channels
  const static int Nch=4;
 };




  

  
