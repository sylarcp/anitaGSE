/****************************************************************************
  Imported from event++ distribution
  Header: /net/local/cvsroot/event++/utilityFunctions.h,v 1.14 2003/07/01 23:10:59 ped Exp 

  $Header: /cvsroot/anitacode/gse/unpack/utilityFunctions.h,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $

  Contains some common command sequences to simplify the code later on.
  Miocinovic 19981113

 ----------------------------------------------------------------------------
  $Log: utilityFunctions.h,v $
  Revision 1.1.1.1  2005/07/20 03:12:18  predragm
  Initial import.

  Revision 1.14  2003/07/01 23:10:59  ped
  Fixes for gcc2/3 cross-compliance

  Revision 1.13  2003/06/29 16:34:53  ped
  Upgraded to compile with gcc3

  Revision 1.12  2003/01/30 19:11:04  ped
  Upgraded library to F2K v1.5 level. Fixed handling of extra long lines which break the format.

 * Revision 1.11  2001/07/30  21:26:43  ped
 * Upgraded to gcc-3.0 compliance
 *
 * Revision 1.10  2001/01/25  21:42:23  ped
 * Taken out a comment problematic for DOC++
 *
 * Revision 1.9  1999/06/18  19:39:50  ped
 * Minor bug fixes. Moved some utility functions around. Fixed documentation.
 *
 * Revision 1.8  1999/06/04  17:43:13  ped
 * Added shared library installation support. Added lookup extension to STL vector
 * class and reimplemented all data complex data classes with it. Some speed
 * optimization. Various bug fixes and minor changes.
 *
 * Revision 1.7  1999/05/24  21:56:02  ped
 * Re-check of all files after archive loss. Logs prior to this one are unreliable at best.
 *
 * Revision 1.7  1999/05/21  01:40:48  ped
 * Lot of additions for data interface, some bug fixes, few additional methods,
 * removed copy constructors and assignment operators where not needed.
 *
 * Revision 1.6  1999/04/30  18:27:53  ped
 * Added second level interface
 *
 * Revision 1.5  1999/03/10  23:11:17  ped
 * Autoconf and automake code building added.
 *
 ****************************************************************************/
 
#ifndef UTILITYFUNCTIONS_H
#define UTILITYFUNCTIONS_H


#include <ext/hash_map>
#include <cstdlib>
#include <iostream>
#include <cstring>
using namespace std;


#define UNAVAILABLE_INT     -66666
#define UNAVAILABLE_FLOAT   -66666.
#define UNAVAILABLE_TOK     ""
#define NOT_AVAILABLE_TOK   ""
#define NOT_A_NUMBER        "nan"
#define INFTY_TOK           "inf"
#define NEGATIVE_INFTY_TOK  "-inf"
#define MAX_STR    1024


class Value;

/** Frees up string memory.
 * WARNING: If {\em ptr} is not pointing to a string, {\em Segmentation Fault}
 * will be caused. 0 is accepted, of course. 
 */
inline void CLEAR(char **ptr)
{
  if(*ptr){
    delete [](*ptr);
    *ptr=0;
  }
}

/** Makes a deep copy of a string. It allocates necessary memory and 
 * copies {\em str} to {\em ptr}. */
inline void ASSIGN(char **ptr,const char *str)
{
  CLEAR(ptr);
  if(str){
    *ptr = new char[strlen(str)+1];
    strcpy(*ptr,str);
  }
}

/** Determines if a string is an integer and returns it's value.
 * The numerical value is returned through {\em i}.
 * @return TRUE if {\em tok} is integer, FALSE otherwise. */
inline bool isInt(const char *tok,int &i)
{
  char *remainder;

  if(tok==0) return false;

  i=strtol(tok,&remainder,10);
  if((*remainder!='\0') && (*remainder != '\n')){
    i=UNAVAILABLE_INT;
    return false;
  }
  else
    return true;
}

/** Determines if a string is a float and returns it's value.
 * The numerical value is returned through {\em f}.
 * @return TRUE if {\em tok} is float, FALSE otherwise. */
inline bool isFloat(const char *tok,float &f)
{
  char *remainder;
  
  if(tok==0) return false;

  f=strtod(tok,&remainder);
  if((*remainder!='\0') && (*remainder != '\n')){
    f=UNAVAILABLE_FLOAT;
    return false;
  }
  else
    return true;
}

/** Safely compares two strings for equality. 
 *  @return 0 if strings are not equal, non-zero if the strings are equal.
 */
inline int SAFE_STRCMP(const char *s1,const char *s2){
  if(s1==s2)       // They are either 0 or point to same string
    return 1;
  if((!s1)||(!s2)) // One of them is 0
    return 0;
  return !strcmp(s1,s2);
}

/* Hash function implementation for Value class. */
namespace __gnu_cxx{
template<> struct hash<Value>{
  /** This routine implements hashing of Value class variables. It is to be 
   * used with STL hashed containers when Value class variable is a key. 
   *  NOTE: This is a very simplistic hash and it's not optimized for any
   *  serious hash use.
   *  @param Value Value to be hashed
   *  @return Hash result */
  size_t operator() (const Value &) const;
  };
}

#endif // UTILITYFUNCTIONS_H






