/****************************************************************************
  Imported from event++ distribution:
  Header: /net/local/cvsroot/event++/word.h,v 1.10 1999/06/18 19:39:52 ped Exp 

  $Header: /cvsroot/anitacode/gse/unpack/word.h,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $

  Defines a word, which is a value combined with a proper name
  Miocinovic 19981113

 ----------------------------------------------------------------------------
  $Log: word.h,v $
  Revision 1.1.1.1  2005/07/20 03:12:18  predragm
  Initial import.

  Revision 1.10  1999/06/18 19:39:52  ped
  Minor bug fixes. Moved some utility functions around. Fixed documentation.

 * Revision 1.9  1999/06/04  17:43:18  ped
 * Added shared library installation support. Added lookup extension to STL vector
 * class and reimplemented all data complex data classes with it. Some speed
 * optimization. Various bug fixes and minor changes.
 *
 * Revision 1.8  1999/05/24  21:56:12  ped
 * Re-check of all files after archive loss. Logs prior to this one are unreliable at best.
 *
 * Revision 1.8  1999/05/21  01:40:52  ped
 * Lot of additions for data interface, some bug fixes, few additional methods,
 * removed copy constructors and assignment operators where not needed.
 *
 * Revision 1.7  1999/04/30  18:27:57  ped
 * Added second level interface
 *
 * Revision 1.6  1999/03/10  23:11:21  ped
 * Autoconf and automake code building added.
 *
 ****************************************************************************/

#ifndef WORD_H
#define WORD_H

#include "value.h"

/**
 * Simple word, for use as a basic storage device for f2k values.
 *
 * In essence we just add a string name to a \Ref{Value}, but it we can also 
 * use it just to store field names or just to store variable values.
 */
class Word:public Value{
public: 
  /** Constructor.
   *  @param char word tag, i.e. name */
  Word(const char * = 0);

  /// An alternate constructor
  Word(const Value &,const char * = 0);

  /// Destructor
  virtual ~Word();

  /// Copy Constructor
  Word(const Word &);

  /// Assignment operator
  Word& operator = (const Word &);

  /// Equality operator
  bool operator == (const Word &) const;
  /// Inequality operator
  bool operator != (const Word &) const;

  /// Clear the word to the original unset state
  virtual void clear();

  /// Set word name
  inline void setTag(const char *str){ASSIGN(&tag,str);}

  /** Get pointer to the word name.
   *  @return Pointer to the word name, 0 if there is no name. */
  inline const char *getTag() const{return tag;}

  /// Set Value by assignment
  inline Value &operator = (const Value &v){return Value::operator=(v);}
  
  /** @name Assignment operators 
   * @memo These methods are reimplemented from Value class since we would 
   * like to return value assigned in its original state.
   */
  //@{
  /// Set integer value by assignment
  inline int operator = (int i){return Value::operator=(i);}

  /// Set float value by assignmet
  inline float operator = (float f){return Value::operator=(f);}

  /// Set string value by assignment
  inline const char *operator = (const char *s){return Value::operator=(s);}
  //@}

  /** Input tokens for reassign method.
   * \begin{itemize}
   * \item TagNeeded - If there is no tag, make current string value a tag
   * \item ValueNeeded - If there is no value, make current tag value. You 
   * should NEVER have to use this call, but it it is provided for 
   * completeness.
   * \item BothNeeded - Value contains both tag and value, separated by a 
   * specified character
   * \end{itemize} */
  enum {
    TagNeeded=0x01,     
    ValueNeeded=0x02,
    BothNeeded=0x04
  };

  /** Reassigns Word tag and value. Used during input parsing.
   *  This function is to be used ONLY during the initial parsing 
   *  and assignment of read F2K lines. It reassigns between a value and 
   *  a tag. It also splits a value into a tag and a value using the given
   *  character as a separator.
   *  @return TRUE is requested reassignment was successful, 0 otherwise. */
   bool reassign(int,char = '=');

private:
  /// Name of a word
  char *tag;
};

#endif // WORD_H







