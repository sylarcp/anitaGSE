/****************************************************************************
  Imported from event++ distribution:
  Header: /net/local/cvsroot/event++/word.cc,v 1.6 2003/06/29 16:34:53 ped Exp

  $Header: /cvsroot/anitacode/gse/unpack/word.cc,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $

  Definitions of member functions of Word class
  Miocinovic 19981113

 ----------------------------------------------------------------------------
  $Log: word.cc,v $
  Revision 1.1.1.1  2005/07/20 03:12:18  predragm
  Initial import.

  Revision 1.6  2003/06/29 16:34:53  ped
  Upgraded to compile with gcc3

  Revision 1.5  1999/06/04 17:43:17  ped
  Added shared library installation support. Added lookup extension to STL vector
  class and reimplemented all data complex data classes with it. Some speed
  optimization. Various bug fixes and minor changes.

// Revision 1.4  1999/05/24  21:56:11  ped
// Re-check of all files after archive loss. Logs prior to this one are unreliable at best.
//
// Revision 1.4  1999/05/21  01:40:52  ped
// Lot of additions for data interface, some bug fixes, few additional methods,
// removed copy constructors and assignment operators where not needed.
//
// Revision 1.3  1999/04/30  18:27:56  ped
// Added second level interface
//
// Revision 1.2  1999/03/10  23:11:20  ped
// Autoconf and automake code building added.
//
 ****************************************************************************/

#include "word.h"

// Constructor
Word::Word(const char *str):Value(),tag(0) {ASSIGN(&tag,str);}

// An alternate constructor
Word::Word(const Value &v,const char *str):Value(v),tag(0){
  ASSIGN(&tag,str);
}

// Destructor
Word::~Word() {CLEAR(&tag);}

// Copy Constructor
Word::Word(const Word &old_word):Value((Value)old_word),tag(0){  	    
  ASSIGN(&tag,old_word.tag);
}

// Assignment operator
Word &Word::operator = (const Word &old_word){
  if(this==&old_word) return *this;
  Value::operator=((Value)old_word);
  ASSIGN(&tag,old_word.tag);
  return *this;
}

// Equality operator
bool Word::operator == (const Word &w) const{
  if(SAFE_STRCMP(tag,w.tag)&&(Value::operator==((Value)w)))
    return true;
  else
    return false;
}

// Inequality operator
bool Word::operator != (const Word &w) const{
  return !operator==(w);
}

// Clear the word
void Word::clear(){
  Value::clear();
  CLEAR(&tag);
}

// Reassigns Word tag and value
bool Word::reassign(int req,char sep){
  switch(req){
  case TagNeeded:
    if((!tag)&&(type()==String)){
      get(&tag);
      return true;
    }else
      return false;
    break;
  case ValueNeeded:
    if((!isAvailable())&&(tag)){
      set(tag);
      return true;
    }else
      return false;
    break;
  case BothNeeded:
    if((!tag)&&(type()==String)){
      int i=-1;
      char *val=0;   // Local copy of value.s from Value class
      get(&val);
      while((val[++i]!='\0')&&(val[i]!=sep));
      if(i<strlen(val)){ // Separation character found
	parse(&val[i+1]);  // Now determite type of value and assign it 
	val[i]='\0';
	ASSIGN(&tag,val);  // Extract tag
	return true;
      }else
	return false;
    }else
      return false;
    break;
  default:
    return false;
  }
}


