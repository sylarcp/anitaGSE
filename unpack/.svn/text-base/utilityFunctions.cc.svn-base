/****************************************************************************
  $Header: /cvsroot/anitacode/gse/unpack/utilityFunctions.cc,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $

  Contains some routines that didn't find a better place somewhere else
  Miocinovic 19981113

 ----------------------------------------------------------------------------
  $Log: utilityFunctions.cc,v $
  Revision 1.1.1.1  2005/07/20 03:12:18  predragm
  Initial import.

  Revision 1.5  2003/07/01 23:10:59  ped
  Fixes for gcc2/3 cross-compliance

  Revision 1.4  2003/06/29 16:34:53  ped
  Upgraded to compile with gcc3

  Revision 1.3  1999/06/18 19:39:49  ped
  Minor bug fixes. Moved some utility functions around. Fixed documentation.

 ****************************************************************************/
 
#include "value.h"

// Value hash function
using namespace __gnu_cxx;
size_t hash<Value>::operator()(const Value &v) const{
  size_t result;
  int tmp_i;
  double tmp_f;
  
  if(!v.isAvailable()){
    result = 0;
  }else{
    switch(v.type()){
    case Value::Int:{
      v.get(tmp_i);
      if(tmp_i<0)
	result = tmp_i * -1;
      else
	result = tmp_i;
      break;
    }
    case Value::Float:{
      v.get(tmp_f);
      if(tmp_f<0)
	result = (size_t)(tmp_f * -1);
      else
	result = (size_t)tmp_f;
      break;
    }
    case Value::String:{
      result = v.getString()[0];
      break;
    }
    default:{
      result = 0;
      break;
    }
    }
  }
  return result;
}

