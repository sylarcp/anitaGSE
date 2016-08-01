/* 
 *  Class declaration for object handling storage of packets into files
 */

#include <string>
using std::string;

extern "C"{
#include <stdio.h>
#include <zlib.h>
#include "../lib/anitaGSE.h"
#include <includes/anitaStructures.h>
}

class fileHandler{
 public:
  fileHandler(const char *dir_p);
  ~fileHandler();

  bool store(const void *pckStruct_p);

  bool openNewFile(int type,const void *pckStruct_p);
  bool openTextFile(int type,const void *pckStruct_p);
  bool openPedestalFile(int type,const void *pckStruct_p,const char *fmt="rb");
  bool openWaveformFile(int type,const void *pckStruct_p);
  bool openHKCalibFile(int type,const void *pckStruct_p);

 protected:
  void makeDir(const string dir); // Make directory (equivalent to 'mkdir -p')
  bool storePedestal(const void *pckStruct_p); // Pedestals are handled completely differently
  bool storeTextFile(const void *pckStruct_p); // Zipped text file are handled yet completely differently again
  string typeDir(int type);
  FILE *fp[N_PCKTYPE];
  string currentFile[N_PCKTYPE];
  int currentCount[N_PCKTYPE];

 private:
  string basedir;
};
