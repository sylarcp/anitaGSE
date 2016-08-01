/****************************************************************************

  Imported fron event++ distribution:
  Header: /net/local/cvsroot/event++/value.h,v 1.10 1999/06/18 19:39:51 ped Exp 
  $Header: /cvsroot/anitacode/gse/unpack/value.h,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $

  Defines the most basic storage class, a data value
  Miocinovic 19981113

 ----------------------------------------------------------------------------
  $Log: value.h,v $
  Revision 1.1.1.1  2005/07/20 03:12:18  predragm
  Initial import.

  Revision 1.10  1999/06/18 19:39:51  ped
  Minor bug fixes. Moved some utility functions around. Fixed documentation.

 * Revision 1.9  1999/06/04  17:43:15  ped
 * Added shared library installation support. Added lookup extension to STL vector
 * class and reimplemented all data complex data classes with it. Some speed
 * optimization. Various bug fixes and minor changes.
 *
 * Revision 1.8  1999/05/24  21:56:04  ped
 * Re-check of all files after archive loss. Logs prior to this one are unreliable at best.
 *
 * Revision 1.9  1999/05/21  01:40:50  ped
 * Lot of additions for data interface, some bug fixes, few additional methods,
 * removed copy constructors and assignment operators where not needed.
 *
 * Revision 1.8  1999/05/05  21:45:36  ped
 * Restructure top level interface to use templates.
 *
 * Revision 1.7  1999/04/30  18:27:54  ped
 * Added second level interface
 *
 * Revision 1.6  1999/03/10  23:11:19  ped
 * Autoconf and automake code building added.
 *
 ****************************************************************************/

#ifndef VALUE_H
#define VALUE_H

#include <cmath>
#include <string>

#include "utilityFunctions.h"

/**
 * Class to be used for storing variable values. It supports integer, float, 
 * and single word strings. 
 *
 * I'd like to hide all grizzly details about variables in this class.
 * I know that there might be some special cases, which can be handled 
 * separately or included in this class in some clever way.
 */
class Value{
public:
  /** Union where the actual variable of Value class lives. The type of value 
   * stored is kept in {\em valueType} field of \Ref{Value} class.
   */
  union ValueStorage{  
    int i;
    float f;
    char *s;
  };

  /// Default constructor
  Value();

  /**@name Initializing constructors */
  //@{
  /// Integer initialization
  Value(const int);  
  /// Unsigned integer initialization
  Value(const unsigned int);  
  /// Long integer initialization
  Value(const long);  
  /// Unsigned long integer initialization
  Value(const unsigned long);  
  /// Real number initialization
  Value(const float);
  /// String initialization
  Value(const char *);
  //@}

  /// Destructor
  virtual ~Value();

  /// Copy Constructor
  Value(const Value &);

  /// Assignment operator
  Value &operator = (const Value &);

  /** Enumerator that defines the variable types that can be stored in Value 
   * class.
   * \begin{itemize}
   * \item Int - int 
   * \item Float - float
   * \item String - character pointer
   * \end{itemize}
   * Note that {\em String} is not standard C++ String class, but just locally 
   * implemented {\em char *} type.
   */ 
  enum {Int, Float, String};

  /// Clears Value to the original unset state
  virtual void clear();

  /** Get current vartiable type
   *  @return The type of variable stored */
  inline int type() const {return valueType;} 

  /**@name Value setting methods */
  //@{

  /// Sets integer value 
  void set(const int);

  /// Sets integer by assignment
  int operator = (const int);

  /// Sets unsigned integer value 
  void set(const unsigned int);

  /// Sets unsigned integer by assignment
  int operator = (const unsigned int);

  /// Sets long integer value 
  void set(const long);

  /// Sets long integer by assignment
  int operator = (const long);

  /// Sets unsigned long integer value 
  void set(const unsigned long);

  /// Sets unsigned long integer by assignment
  int operator = (const unsigned long);

  /// Sets float value
  void set(const float);

  /// Sets float value
  void set(const double);

  /// Sets float by assignmet
  float operator = (const float);

  /** An alternate way to set infinity. 
   *  The argument indicates a positive or negative infinity. */
  void setInfinity(int = 1);

  /// Sets string value
  void set(const char *);

  /// Sets string by assignment
  const char *operator = (const char *);

  /// Parses the string to determine the type of variable and stores it
  void parse(const char *);

  //@}

  /**@name Value extraction methods */
  //@{

  /** Get current value through the integer argument.
   *  @return Current value through the argument or UNAVAILABLE\_INT if
   *  unavailable or String  */
  void get(int &) const;

  /** Get current value through the long integer argument.
   *  @return Current value through the argument, or UNAVAILABLE\_INT if
   *  unavailable or String  */
  void get(long &) const;

  /** Get current value as integer.
   *  @return Current value, or UNAVAILABLE\_INT if unavailable or String  */
  int getInt() const;

  /** Get current value through float argument.
   *  @return Current value through the argument or UNAVAILABLE\_FLOAT if
    *  unavailable or String  */
  void get(float &) const;
 
  /** Get current value through double argument.
   *  @return Current value through the argument or UNAVAILABLE\_FLOAT if
   *   unavailable or String  */ 
  void get(double &) const;
 
  /** Get current value as a float.
   *  @return Current value or UNAVAILABLE\_FLOAT if unavailable or String. */
  float getFloat() const;

  /** Get current value through a string argument.
   *  @return current value through the argument, UNAVAILABLE\_TOK if 
   *  unavailable.
   *  WARNING: Make sure that {\em str} is cleared before sending it here.
   *  Otherwise, there will be memory leaks!
   */
  void get(char **) const;
  
  /** Get a pointer to a string of the current value.
   *  WARNING: This is not the location of the actual Value!
   *  @return Pointer to a string of the current value or UNAVAILABLE\_TOK if 
   *  unavailable. */
  const char *getString() const;

  /** Status of availability flag.
   *  @return TRUE if some type of value is available, FALSE otherwise. */
  inline bool isAvailable() const {return available;}

  /** Status of infinity flag.
   *  @return TRUE if the value is infinite, FALSE otherwise. */
  inline bool isInfinite() const {return infinity;}

  //@}

  /**@name Comparison operators */
  //@{

  /** Equality operator. It compares objects for exact equality, not
   * numerical or lexical equality. See \Ref{equal}. */
  bool operator == (const Value &) const;

  /// Inequality operator
  bool operator != (const Value &) const;

  /** Check for numerical/lexical equality. 
   * @param Value to compare
   * @return TRUE if string representations of two values are equal, or if
   * numerical values are equal, FALSE otherwise */
  bool equal(const Value &) const;

  /** Equality comparison operator between Value and integer.
   *  WARNING: If you compare Value that doesn't store integer, 
   *  UNAVAILABLE\_INT will be used for comparison.
   *  @return 1 if integer Value stored is equal to integer compared to, 0
   *  otherwise. */
  inline int operator == (const int i) const{
    return getInt()==i;
  }

  /** Inequality comparison operator between Value and integer.
   *  WARNING: If you compare Value that doesn't store integer, 
   *  UNAVAILABLE\_INT will be used for comparison.
   *  @return 1 if integer Value stored is not equal to integer compared to, 0
   *  otherwise. */
  inline int operator != (const int i) const{
    return getInt()!=i;
  }

  /** Greater than or equal comparison operator between Value and integer.
   *  WARNING: If you compare Value that doesn't store integer, 
   *  UNAVAILABLE\_INT will be used for comparison.
   *  @return 1 if integer Value stored is greater than or equal to integer 
   *  compared to, 0 otherwise. */
  inline int operator >= (const int i) const{
    return getInt()>=i;
  }

  /** Less than or equal comparison operator between Value and integer.
   *  WARNING: If you compare Value that doesn't store integer, 
   *  UNAVAILABLE\_INT will be used for comparison.
   *  @return 1 if integer Value stored is less then or equal to integer 
   *  compared to, 0 otherwise. */
  inline int operator <= (const int i) const{
    return getInt()<=i;
  }

  /** Greater than comparison operator between Value and integer.
   *  WARNING: If you compare Value that doesn't store integer, 
   *  UNAVAILABLE\_INT will be used for comparison.
   *  @return 1 if integer Value stored is greater than integer compared to, 0
   *  otherwise. */
  inline int operator > (const int i) const{
    return getInt()>i;
  }

  /** Less than comparison operator between Value and integer.
   *  WARNING: If you compare Value that doesn't store integer, 
   *  UNAVAILABLE\_INT will be used for comparison.
   *  @return 1 if integer Value stored is less than integer compared to, 0
   *  otherwise. */
  inline int operator < (const int i) const{
    return getInt()<i;
  }

  /** Equality comparison operator between Value and float.
   *  WARNING: If you compare Value that doesn't store integer, 
   *  UNAVAILABLE\_FLOAT will be used for comparison.
   *  @return 1 if float Value stored is equal to float compared to,
   *  0 otherwise. */
  inline int operator == (const float f) const{
    return getFloat()==f;
  }

  /** Inequality comparison operator between Value and float.
   *  WARNING: If you compare Value that doesn't store float, 
   *  UNAVAILABLE\_FLOAT will be used for comparison.
   *  @return 1 if float Value stored is not equal to float compared to,
   *  0 otherwise. */
  inline int operator != (const float f) const{
    return getFloat()!=f;
  }

  /** Greater than or equal comparison operator between Value and float.
   *  WARNING: If you compare Value that doesn't store float, 
   *  UNAVAILABLE\_FLOAT will be used for comparison.
   *  @return 1 if float Value stored is greater than or equal to float 
   *  compared to, 0 otherwise. */
  inline int operator >= (const float f) const{
    return getFloat()>=f;
  }

  /** Less than or equal comparison operator between Value and float.
   *  WARNING: If you compare Value that doesn't store float, 
   *  UNAVAILABLE\_FLOAT will be used for comparison.
   *  @return 1 if float Value stored is less than or equal to float 
   *  compared to, 0 otherwise. */
  inline int operator <= (const float f) const{
    return getFloat()<=f;
  }

  /** Greater than comparison operator between Value and float.
   *  WARNING: If you compare Value that doesn't store float, 
   *  UNAVAILABLE\_FLOAT will be used for comparison.
   *  @return 1 if float Value stored is greater than float compared to,
   *  0 otherwise. */
  inline int operator > (const float f) const{
    return getFloat()>f;
  }

  /** Less than comparison operator between Value and float.
   *  WARNING: If you compare Value that doesn't store float, 
   *  UNAVAILABLE\_FLOAT will be used for comparison.
   *  @return 1 if float Value stored is less than float compared to,
   *  0 otherwise. */
  inline int operator < (const float f) const{
    return getFloat()<f;
  }

  /** Greater than or equal comparison operator between Values.
   *  WARNING: If you compare Value that don't store same type of variable, 
   *  UNAVAILABLE value will be used for comparison.
   *  @return 1 if Value stored is greater than or equal to Value 
   *  compared to, 0 otherwise. */
  inline int operator >= (const Value &v) const{
    switch(v.type()){
    case Int: return getInt()>=v.getInt();
    case Float: return getFloat()>=v.getFloat();
    case String: return strcmp(getString(),v.getString()) >= 0;
    default: return 0;
    }
  }

  /** Less than or equal comparison operator between Values.
   *  WARNING: If you compare Value that don't store same type of variable, 
   *  UNAVAILABLE value will be used for comparison.
   *  @return 1 if Value stored is less than or equal to Value 
   *  compared to, 0 otherwise. */
  inline int operator <= (const Value &v) const{
    switch(v.type()){
    case Int: return getInt()<=v.getInt();
    case Float: return getFloat()<=v.getFloat();
    case String: return strcmp(getString(),v.getString()) <= 0;
    default: return 0;
    }
  }

  /** Greater than comparison operator between Values.
   *  WARNING: If you compare Value that don't store same type of variable, 
   *  UNAVAILABLE value will be used for comparison.
   *  @return 1 if Value stored is greater than Value 
   *  compared to, 0 otherwise. */
  inline int operator > (const Value &v) const{
    switch(v.type()){
    case Int: return getInt()>v.getInt();
    case Float: return getFloat()>v.getFloat();
    case String: return strcmp(getString(),v.getString()) > 0;
    default: return 0;
    }
  }

  /** Less than comparison operator between Values.
   *  WARNING: If you compare Value that don't store same type of variable, 
   *  UNAVAILABLE value will be used for comparison.
   *  @return 1 if Value stored is less than Value 
   *  compared to, 0 otherwise. */
  inline int operator < (const Value &v) const{
    switch(v.type()){
    case Int: return getInt()<v.getInt();
    case Float: return getFloat()<v.getFloat();
    case String: return strcmp(getString(),v.getString()) < 0;
    default: return 0;
    }
  }

  //@}

  /**@name Arithmetic operators */
  //@{
   
  /** Addition operator. 
   * If {\em Float} and {\em Int} are added, the result is {\em Float}.
   * If {\em String}s are added, they are concated.
   * If {\em Float} or {\em Int} is added to a {\em String}, numerical value 
   * is converted to a {\em String} first. */
  Value &operator+=(const Value &);

  /** Subtraction operator.
   * If {\em Float} and {\em Int} are subtracted, the result is {\em Float}.
   * If {\em String} is one of the arguments, the operation is silently 
   * ignored and the Value is not changed. */
  Value &operator-=(const Value &);

  /** Multiplication operator.
   * If {\em Float} and {\em Int} are multiplied, the result is {\em Float}.
   * If {\em String} is one of the arguments and {\em Int} {\em n}>0 is the 
   * other, the {\em String} is concated with itself {\em n} times. */
  Value &operator*=(const Value &);

  /** Division operator.
   * If {\em Float} and {\em Int} are divided, the result is {\em Float}.
   * If {\em String} is one of the arguments, the operation is silently 
   * ignored and the Value is not changed. */
  Value &operator/=(const Value &);

  /** Modulus operator.
   * If both arguments are not {\em Int}, the operation is silently ignored
   * and the Value is not changed. */
  Value &operator%=(const Value &);

  /** Addition operator. 
   * If Value is {\em String}, numerical value is converted to a {\em String} 
   * and then concated. */
  Value &operator+=(const int);

  /** Subtraction operator. 
   * If Value is {\em String}, the operation is silently ignored and the Value 
   * is not changed. */
  Value &operator-=(const int);

  /** Multiplication operator. 
   * If Value is {\em String}, it will be concated with itself {\em int}
   * times. */
  Value &operator*=(const int);

  /** Division operator. 
   * If Value is {\em String}, the operation is silently ignored and the Value 
   * is not changed. */
  Value &operator/=(const int);

  /**  Modulus operator.
   * If Value is not {\em Int}, the operation is silently ignored and the 
   * Value not changed. */
  Value &operator%=(const int);
  
  /** Addition operator. 
   * If Value is {\em String}, numerical value is converted to a String and
   * then concated. */
  Value &operator+=(const float);

  /** Subtraction operator. 
   * If Value is {\em String}, the operation is silently ignored and the Value 
   * is not changed.*/   
  Value &operator-=(const float);

  /** Multiplication operator. 
   * If Value is {\em String}, the operation is silently ignored and the Value 
   * is not changed.*/   
  Value &operator*=(const float);

  /** Division operator. 
   * If Value is {\em String}, the operation is silently ignored and the Value 
   * is not changed.*/   
  Value &operator/=(const float);
  
  /** Addition operator.
   * If Value is {\em String}s, the character array will be concated.
   * If Value is {\em Int} or {\em Float}, it will be converted to 
   * {\em String}, and the character array will be concated. */
  Value &operator+=(const char *);

  //@}

  /** @name Casting operators */
  //@{

  /// Returns Value in integer format.
  inline operator int () const{return getInt();}
  /// Returns Value in long integer format.
  inline operator long () const{return long(getInt());}
  /// Returns Value in short integer format.
  inline operator short () const{return short(getInt());}
  /// Returns Value in unsigned integer format.		
  inline operator unsigned () const{return unsigned(getInt());}
  /// Return Value in long unsigned integer format
  inline operator long unsigned () const{return (long unsigned)getInt();}  

  /// Returns Value in double format.
  inline operator double () const{return double(getFloat());}
  /// Returns Value in float format.
  inline operator float () const{return getFloat();}

  /// Returns Value in string format.
  inline operator const char * () const{return getString();}

  //@}

  /**@name Friend functions */
  //@{

  /// Output operator
  friend ostream &operator << (ostream &,const Value &);
  /// Input operator
  friend istream &operator >> (istream &,Value &);

  /** Addition operator.
   * If {\em Float} and {\em Int} are added, the result is {\em Float}.
   * If {\em String}s are added, they are concated.
   * If {\em Float} or {\em Int} is added to a {\em String}, numerical value 
   * is converted to a {\em String} first. 
   * @return Value with the result of the operation. */ 
  friend Value operator + (const Value &,const Value &); 

  /** Subtraction operator.
   * If {\em Float} and Int are subtracted, the result is {\em Float}.
   * If {\em String} is one of the arguments, the operation is silently 
   * ignored and the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator - (const Value &,const Value &); 

  /** Multiplication operator.
   * If {\em Float} and {\em Int} are multiplied, the result is {\em Float}.
   * If {\em String} is one of the arguments and {\em Int} {\em n}>0 is the 
   * other, the {\em String} is concated with itself {\em n} times.
   * @return Value with the result of the operation. */
  friend Value operator * (const Value &,const Value &); 

  /** Division operator.
   * If {\em Float} and {\em Int} are divided, the result is {\em Float}.
   * If {\em String} is one of the arguments, the operation is silently 
   * ignored and the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator / (const Value &,const Value &); 

  /** Modulus operator.
   * If both arguments are not {\em Int}, the operation is silently ignored 
   * and the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator % (const Value &,const Value &); 

  /** Addition operator. 
   * If Value is {\em String}, numerical value is converted to a {\em String} 
   * and then concated.
   * @return Value with the result of the operation. */
  friend Value operator + (const Value &,const int); 

  /** Subtraction operator. 
   * If Value is {\em String}, the operation is silently ignored and 
   * the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator - (const Value &,const int); 

  /** Multiplication operator. 
   * If Value is {\em String}, it will be concated with itself {\em int} times.
   * @return Value with the result of the operation. */
  friend Value operator * (const Value &,const int); 

  /** Division operator. 
   * If Value is {\em String}, the operation is silently ignored and 
   * the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator / (const Value &,const int); 

  /**  Modulus operator.
   * If Value is not {\em Int}, the operation is silently ignored and 
   * the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator % (const Value &,const int); 

  /** Addition operator. 
   * If Value is {\em String}, numerical value is converted to a {\em String} 
   * and then concated.
   * @return Value with the result of the operation. */ 
  friend Value operator + (const Value &,const float); 

  /** Subtraction operator. 
   * If Value is {\em String}, the operation is silently ignored and 
   * the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator - (const Value &,const float); 

  /** Multiplication operator. 
   * If Value is {\em String}, the operation is silently ignored and 
   * the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator * (const Value &,const float); 

  /** Division operator. 
   * If Value is {\em String}, the operation is silently ignored and 
   * the result is undefined Value. 
   * @return Value with the result of the operation. */
  friend Value operator / (const Value &,const float); 

  /** Addition operator.
   * If Value is {\em String}, the character array will be concated.
   * If Value is {\em Int} or {\em Float}, it will be converted to 
   * {\em String}, and the character array will be concated. 
   * @return Value with the result of the operation. */
  friend Value operator + (const Value &,const char *); 

  //@}
private:
  /// Type of variable this class stores
  int valueType;     

  /// Value of the variable stored
  ValueStorage value;               

  /// Is variable meaningfull
  bool available;                   

  /// Is variable infinite
  bool infinity;                    
};

/** @name Functions associated with Value class */
//@{
/// Output operator for a pointer to a Value
ostream &operator << (ostream &,const Value *);

/** Input operator for a pointer to a Value class object. Use this function
 *  with caution. Memory necessary will be allocated here, so you 
 *  should send only uninitialized pointers. */
istream &operator >> (istream &,Value *);

/** Comparison operator between Value and string.
 *  WARNING: If you compare Value that stores a number, the number will be 
 *  converted into a string and than compared.
 *  @return Same type of return as built-in strcmp. */
int strcmp(const Value &,const char *);

/** Comparison operator between string and Value.
 *  WARNING: If you compare Value that stores a number, the number will be 
 *  converted into a string and than compared.
 *  @return Same type of return as built-in strcmp. */
int strcmp(const char *,const Value &);
//@}

#endif // VALUE_H





