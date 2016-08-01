/****************************************************************************
  Imported from event++ distribution:
  Header: /net/local/cvsroot/event++/value.cc,v 1.9 2003/06/29 16:34:53 ped Exp 
  $Header: /cvsroot/anitacode/gse/unpack/value.cc,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $

  Definitions of member functions of Value class. 


 ----------------------------------------------------------------------------
  $Log: value.cc,v $
  Revision 1.1.1.1  2005/07/20 03:12:18  predragm
  Initial import.

  Revision 1.9  2003/06/29 16:34:53  ped
  Upgraded to compile with gcc3

  Revision 1.8  2003/01/30 19:11:04  ped
  Upgraded library to F2K v1.5 level. Fixed handling of extra long lines which break the format.

// Revision 1.7  1999/10/26  00:05:30  ped
// Upgraded to F2000 v1.4.
//
// Revision 1.6  1999/06/18  19:39:51  ped
// Minor bug fixes. Moved some utility functions around. Fixed documentation.
//
// Revision 1.5  1999/06/04  17:43:14  ped
// Added shared library installation support. Added lookup extension to STL vector
// class and reimplemented all data complex data classes with it. Some speed
// optimization. Various bug fixes and minor changes.
//
// Revision 1.4  1999/05/24  21:56:03  ped
// Re-check of all files after archive loss. Logs prior to this one are unreliable at best.
//
// Revision 1.6  1999/05/21  01:40:49  ped
// Lot of additions for data interface, some bug fixes, few additional methods,
// removed copy constructors and assignment operators where not needed.
//
// Revision 1.5  1999/05/08  01:42:41  ped
// Fixed assignment bug
//
// Revision 1.4  1999/05/05  21:45:35  ped
// Restructure top level interface to use templates.
//
// Revision 1.3  1999/04/30  18:27:54  ped
// Added second level interface
//
// Revision 1.2  1999/03/10  23:11:18  ped
// Autoconf and automake code building added.
//
  Miocinovic 19981117
 ****************************************************************************/

#include <cstdio>
#include "value.h"

#define FLT_MIN 1e-37

// Constructor
Value::Value():valueType(0),available(false),infinity(false){
  value.i=0;
}

// Initializing constructor
Value::Value(const int i):valueType(Int),available(true),infinity(false){
  value.i = i;
  if(i == UNAVAILABLE_INT) available=false;
}

// Initializing constructor
Value::Value(const unsigned int i):valueType(Int),available(true),infinity(false){
  value.i = static_cast<int>(i);
  if(i == UNAVAILABLE_INT) available=false;
}

// Initializing constructor
Value::Value(const long l):valueType(Int),available(true),infinity(false){
  value.i = static_cast<int>(l);
  if(l == UNAVAILABLE_INT) available=false;
}

// Initializing constructor
Value::Value(const unsigned long l):valueType(Int),available(true),infinity(false){
  value.i = static_cast<int>(l);
  if(l == UNAVAILABLE_INT) available=false;
}

// Initializing constructor
Value::Value(const float f):valueType(Float),available(true),infinity(false){
  value.f = f;
  if(f == UNAVAILABLE_FLOAT) available=false;
  if(fabsf(f) == HUGE_VALF) infinity=true;
}

// Initializing constructor  
Value::Value(const char *s):valueType(String),available(true),infinity(false){
  value.s = 0;
  ASSIGN(&value.s,s);
  if(SAFE_STRCMP(s,UNAVAILABLE_TOK)) available=false;
}

// Destructor
Value::~Value(){
  if(valueType==String) CLEAR(&value.s);
}

// Copy Constructor
Value::Value(const Value &old_value){
  valueType=old_value.valueType;
  available=old_value.available;
  infinity=old_value.infinity;
  if(valueType==Int)
    value.i=old_value.value.i;
  else if(valueType==Float)
    value.f=old_value.value.f;
  else{
    value.s=0;
    ASSIGN(&value.s,old_value.value.s);
  }
}

// Assignment operator
Value &Value::operator = (const Value &old_value){
  if(this==&old_value) return *this;
  if((valueType!=String)&&(old_value.valueType==String)) value.s=0;
  valueType=old_value.valueType;
  available=old_value.available;
  infinity=old_value.infinity;
  if(valueType==Int)
    value.i=old_value.value.i;
  else if(valueType==Float)
    value.f=old_value.value.f;
  else
    ASSIGN(&value.s,old_value.value.s);
  return *this;
}

// Clears Value to the original unset state
void Value::clear(){
  valueType=Int;
  available=false;
  infinity=false;
  value.i=0;
}

// Sets integer value 
void Value::set(const int i){                 
  if(valueType==String) CLEAR(&value.s);
  valueType=Int;
  infinity=false;
  available = !(i==UNAVAILABLE_INT);
  value.i=i;
}

// Sets integer by assignment
int Value::operator = (const int i){
  if(valueType==String) CLEAR(&value.s);
  valueType=Int;
  infinity=false;
  available = !(i==UNAVAILABLE_INT);
  value.i=i;
  return i;
}

// Sets unsigned integer value 
void Value::set(const unsigned int i){                 
  if(valueType==String) CLEAR(&value.s);
  valueType=Int;
  infinity=false;
  available = (i!=UNAVAILABLE_INT)?true:false;
  value.i=i;
}

// Sets unsigned integer by assignment
int Value::operator = (const unsigned int i){
  if(valueType==String) CLEAR(&value.s);
  valueType=Int;
  infinity=false;
  available = !(i==UNAVAILABLE_INT);
  value.i=i;
  return i;
}

// Sets long integer value 
void Value::set(const long i){                 
  if(valueType==String) CLEAR(&value.s);
  valueType=Int;
  infinity=false;
  available = !(i==UNAVAILABLE_INT);
  value.i=i;
}

// Sets integer by assignment
int Value::operator = (const long i){
  if(valueType==String) CLEAR(&value.s);
  valueType=Int;
  infinity=false;
  available = !(i==UNAVAILABLE_INT);
  value.i=i;
  return i;
}

// Sets float value
void Value::set(const float f){               
  if(valueType==String) CLEAR(&value.s);
  valueType=Float;
  if((f==HUGE_VALF)||(f==-HUGE_VALF)) 
    infinity=true;
  else
    infinity=false;
  available = !(f==UNAVAILABLE_FLOAT);
  value.f=f;
}

// Sets float value
void Value::set(const double f){               
  if(valueType==String) CLEAR(&value.s);
  valueType=Float;
  if((f==HUGE_VALF)||(f==-HUGE_VALF)) 
    infinity=true;
  else
    infinity=false;
  available = !(f==UNAVAILABLE_FLOAT);
  value.f=(float)f;
}

// Sets float by assignmet
float Value::operator = (const float f){               
  if(valueType==String) CLEAR(&value.s);
  valueType=Float;
  if((f==HUGE_VALF)||(f==-HUGE_VALF))
    infinity=true;
  else
    infinity=false;
  available = !(f==UNAVAILABLE_FLOAT);
  value.f=f;
  return f;
}

// An alternate way to set infinity
void Value::setInfinity(int i){
  if(valueType==String) CLEAR(&value.s);
  valueType=Float;
  available=true;
  infinity=true;
  if(i<0)
    value.f=-HUGE_VALF;
  else
    value.f=HUGE_VALF;
}

// Sets string value
void Value::set(const char *str){             
  if(valueType!=String){ 
    value.s=0;
    valueType=String;
  }
  infinity=false;
  available = !SAFE_STRCMP(str,UNAVAILABLE_TOK);
  ASSIGN(&value.s,str);
}

// Sets string by assignment
const char *Value::operator = (const char *str){
  if(valueType!=String){
    value.s=0;
    valueType=String;
  }
  available = !SAFE_STRCMP(str,UNAVAILABLE_TOK);
  infinity=false;
  ASSIGN(&value.s,str);
  return str;
} 

// Parses the string to determine the type of variable and stores it
void Value::parse(const char *tok){
  int tmp_int;
  float tmp_float;

  if(!strcmp(tok,UNAVAILABLE_TOK)
             ||
     !strcmp(tok,NOT_AVAILABLE_TOK)
             ||
     !strcmp(tok,NOT_A_NUMBER)){
    available = false;
  }else if(!strcmp(tok,INFTY_TOK)){
    set(HUGE_VALF);
  }else if(!strcmp(tok,NEGATIVE_INFTY_TOK)){
    set(-HUGE_VALF);
  }else if(isInt(tok,tmp_int)){
    set(tmp_int);
  }else if(isFloat(tok,tmp_float)){
    set(tmp_float);
  }else{  // Only thing left now is a string
    set(tok);
  }
}


// Get current value through the integer argument.
void Value::get(int &i) const {               
  if(available)
    switch(valueType){
    case Int:{
      i = value.i;
      break;
    }
    case Float:{
      i = (int)value.f;
      break;
    }
    case String:
    default:{
      i = UNAVAILABLE_INT;
      break;
    }
    }
  else
    i = UNAVAILABLE_INT;
}

// Get current value through the long integer argument.
void Value::get(long &i) const {               
  if(available)
    switch(valueType){
    case Int:{
      i = value.i;
      break;
    }
    case Float:{
      i = (long)value.f;
      break;
    }
    case String:
    default:{
      i = UNAVAILABLE_INT;
      break;
    }
    }
  else
    i = UNAVAILABLE_INT;
}

// Get current value as integer.
int Value::getInt() const {
  if(!available) return UNAVAILABLE_INT;
  switch(valueType){
  case Int:
    return value.i;
  case Float:
    return (int)value.f;
  case String:
  default:
    return UNAVAILABLE_INT;
  }
}

// Get current float value through float argument.
void Value::get(float &f) const {             
  if(available)
    switch(valueType){
    case Int:{
      f = (float)value.i;
      break;
    }
    case Float:{
      f = value.f;
      break;
    }
    case String:
    default:{
      f = UNAVAILABLE_FLOAT;
      break;
    }
    }
  else
    f = UNAVAILABLE_FLOAT;
}
 
// Get current float value through double argument.
void Value::get(double &f) const {             
  if(available)
    switch(valueType){
    case Int:{
      f = (double)value.i;
      break;
    }
    case Float:{
      f = (double)value.f;
      break;
    }
    case String:
    default:{
      f = UNAVAILABLE_FLOAT;
      break;
    }
    }
  else
    f = UNAVAILABLE_FLOAT;
}
 
// Get current value as a float.
float Value::getFloat() const{
  if(!available) return UNAVAILABLE_FLOAT;
  switch(valueType){
  case Int:
    return (float)value.i;
  case Float:
    return value.f;
  case String:
  default:
    return UNAVAILABLE_FLOAT;
  }
}

// Get current string value through the argument
void Value::get(char **str) const {   
  char tmp_str[MAX_STR];   // Temporary string

  *str = 0;   /* Note that by this there might be some memory leaks
		 if *str was used previously. This has to be resolved. */

  if(available)
    switch(valueType){
    case Int:{
      sprintf(tmp_str,"%d",value.i);
      ASSIGN(str,tmp_str);
      break;
    }
    case Float:{
      sprintf(tmp_str,"%lf",value.f);
      ASSIGN(str,tmp_str);
      break;
    }
    case String:{
      ASSIGN(str,value.s);
      break;
    }
    default:{
      ASSIGN(str,UNAVAILABLE_TOK);
      break;
    }
    }
  else
    ASSIGN(str,UNAVAILABLE_TOK);
}

// Get a pointer to a string of the current value.
const char *Value::getString() const{
  /* This has very bad implementation that allows only for 5 unique
     calls to this function. A better solution is needed.
     Keep in mind memory leaks, though!
     Better solution is to return C++ string! To be implemented.
  */
  static char tmp_str[5][MAX_STR];   // Temporary strings
  static int count=0;

  if(count>=5) count=0;

  if(available)
    switch(valueType){
    case Int:{
      sprintf(tmp_str[count],"%d",value.i);
      break;
    }
    case Float:{
      sprintf(tmp_str[count],"%lf",value.f);
      break;
    }
    case String:{
      sprintf(tmp_str[count],"%s",value.s);
      break;
    }
    default:{
      sprintf(tmp_str[count],"%s",UNAVAILABLE_TOK);
      break;
    }
    }
  else
    sprintf(tmp_str[count],"%s",UNAVAILABLE_TOK);

  return tmp_str[count++];
} 

// Equality operator for Value class
bool Value::operator == (const Value &v) const{
  if((valueType == v.valueType)&&(available==v.available)){
    switch(valueType){
    case Int:
      return value.i==v.value.i;
    case Float:
      return value.f==v.value.f;
    case String:
      return SAFE_STRCMP(value.s,v.value.s);
    default:
      return false;
    }
  }else
    return false;
}

// Inequality operator for Value class
bool Value::operator != (const Value &v) const{		
  return !operator==(v);
}

// Numerical/lexical equality for Values
bool Value::equal(const Value &v) const{
  if(available&&v.available){ // Both available
    if(valueType==String)
      return !bool(strcmp(*this,v.getString()));
    else
      return this->getFloat()==v.getFloat();
  }else
    return false;
}

// Addition operator
Value &Value::operator+=(const Value &v){
  if(!available||!v.available) return *this;
  if(infinity) return *this;   // Inf + anything = Inf
  if(v.infinity){              
    if(v.value.f>0.0)
      setInfinity(1);
    else
      setInfinity(-1);
    return *this;
  }
  switch(v.valueType){
  case Int:{
    *this+=v.value.i;
    break;
  }
  case Float:{
    *this+=v.value.f;
    break;
  }
  case String:{
    *this+=v.value.s;
    break;
  }
  default:
    break;
  }

  return *this;
}

// Addition operator
Value &Value::operator+=(const int i){
  if(!available||infinity) return *this; // Inf + anything = Inf
  switch(valueType){
  case Int:{
    *this = value.i + i;
    break;
    }
  case Float:{
    *this = value.f + i;
    break;
  }
  case String:{
    char tmp[MAX_STR];
    sprintf(tmp,"%s%d",value.s,i);
    *this = tmp;
    break;
  }
  default:
    break;
  }

  return *this;
}

// Addition operator
Value &Value::operator+=(const float f){
  if(!available||infinity) return *this; // Inf + anything = Inf
  if(fabsf(f)==HUGE_VALF){
    *this = f;
    return *this;
  }
  switch(valueType){
  case Int:{
    *this = value.i + f;
    break;
    }
  case Float:{
    *this = value.f + f;
    break;
  }
  case String:{
    char tmp[MAX_STR];
    sprintf(tmp,"%s%f",value.s,f);
    *this = tmp;
    break;
  }
  default:
    break;
  }

  return *this;
}

// Addition operator
Value &Value::operator+=(const char *str){
  if(!available||infinity) return *this; // Inf + anything = Inf
  static char tmp[MAX_STR];
  switch(valueType){
  case Int:{
      sprintf(tmp,"%d%s",value.i,str);
      *this = tmp;
      break;
    }
    case Float:{
      sprintf(tmp,"%f%s",value.f,str);
      *this = tmp;
      break;
    }
    case String:{
      sprintf(tmp,"%s%s",value.s,str);
      *this = tmp;
      break;
    }
  default:
    break;
  }

  return *this;
}
    
// Subtraction operator
Value &Value::operator-=(const Value &v){
  if(!available||!v.available) return *this;
  if(infinity) return *this; // Inf - anything = Inf
  if(v.infinity){
    if(v.value.f>0.0)
      setInfinity(-1);
    else
      setInfinity(1);
    return *this;
  }
  switch(v.valueType){
  case Int:{
    *this -= v.value.i;
    break;
  }
  case Float:{
    *this -= v.value.f;
    break;
  }
  case String:
  default:
    break;
  }

  return *this;
}
    
// Subtraction operator
Value &Value::operator-=(const int i){
  if(!available||infinity) return *this; // Inf - anything = Inf
  switch(valueType){
  case Int:{
    *this = value.i - i;
      break;
    }
  case Float:{
    *this = value.f - i;
    break;
  }
  case String:
  default:
    break;
  }

  return *this;
}

// Subtraction operator
Value &Value::operator-=(const float f){
  if(!available||infinity) return *this; // Inf - anything = Inf
  if(fabsf(f)==HUGE_VALF){
    *this = -f;
    return *this;
  }  
  switch(valueType){
  case Int:{
    *this = value.i - f;
    break;
  }
  case Float:{
    *this = value.f - f;
    break;
  }
  case String:
  default:
    break;
  }

  return *this;
}
    
// Multiplication operator
Value &Value::operator*=(const Value &v){
  if(!available||!v.available) return *this;
  if(infinity){
    if(!((v.valueType==String)||(v>(float)0.0)))
      value.f*=-1;
    return *this;
  }
  if(v.infinity){
    if(((v.value.f>(float)0.0)&&(*this>(float)0.0))||((v.value.f<(float)0.0)&&(*this<float(0.0))))
      setInfinity(1);
    else
      setInfinity(-1);
    return *this;
  }
  switch(v.valueType){
  case Int:{
    *this *= v.value.i;
    break;
  }
  case Float:{
    *this *= v.value.f;
      break;
    }
  case String:
  default:
    break;
  }

  return *this;
}
  
// Multiplication operator
Value &Value::operator*=(const int i){
  if(!available) return *this;
  if(infinity){
    if(i<0){
      value.f *= -1;
    }else if(i==0){
      clear();         // Inf * 0 = undefined
    }
    return *this;
  }
  switch(valueType){
  case Int:{
    *this = value.i * i;
    break;
  }
  case Float:{
    *this = value.f * i;
    break;
  }
  case String:{
    char tmp[MAX_STR];
    if(i){
      strcpy(tmp,value.s);
      for(int c=0;c<i;++c)
	strcat(tmp,value.s);
    }else{
      *tmp = '\0';
    }
    *this = tmp;
    break;
  }
  default:
    break;
  }
  
  return *this;
}

// Multiplication operator
Value &Value::operator*=(const float f){
  if(!available) return *this;
  if(valueType==String) return *this;
  if(infinity){
    if(f<0.0){
      value.f *= -1;
    }else if(f==0.0){
      clear();         // Inf * 0 = undefined
    }
    return *this;
  }
  if(fabsf(f)==HUGE_VALF){
    if(*this==(float)0.0){
      clear();        // 0 * Inf = undefined
    }else if(((*this>(float)0.0)&&(f>(float)0.0))||((*this<(float)0.0)&&(f<(float)0.0))){
      setInfinity(1);
    }else{
      setInfinity(-1);
    }
    return *this;
  }

  switch(valueType){
  case Int:{
    *this = value.i * f;
    break;
  }
  case Float:{
    *this = value.f * f;
    break;
  }
  case String:
  default:
    break;
  }
  
  return *this;
}
  
// Division operator
Value &Value::operator/=(const Value &v){
  if(!available||!v.available) return *this;
  if((valueType!=String)&&(v.valueType!=String)&&(v==(float)0.0)){
    if(*this>(float)0.0){
      setInfinity(1);      
    }else if(*this<(float)0.0){
      setInfinity(-1);
    }else{
      clear();             // 0 / 0 = undefined
    }
    return *this;
  }
  if(infinity){
    if(!((v.valueType==String)||(v>(float)0.0)))
      value.f*=-1;
    return *this;
  }
  if(v.infinity){
    if(valueType!=String)
      *this = 0;
    return *this;
  }
  switch(v.valueType){
  case Int:{
    *this /= v.value.i;
    break;
  }
  case Float:{
    *this /= v.value.f;
    break;
  }
  case String:
  default:
    break;
  }
  
  return *this;
}
   
// Division operator
Value &Value::operator/=(const int i){
  if(!available) return *this;
  if(infinity){
    if(i<0) value.f *= -1;
    return *this;
  }
  switch(valueType){
  case Int:{
    *this = value.i / i;
    break;
  }
  case Float:{
    *this = value.f / i;
    break;
  }
  case String:
  default:
    break;
  }

  return *this;
}

// Division operator
Value &Value::operator/=(const float f){
  if(!available) return *this;
  if(infinity){
    if(f<(float)0.0) value.f *= -1;
    return *this;
  }
  if(fabsf(f)==HUGE_VALF){
    if(valueType!=String)
      *this = 0;
    return *this;
  }
  switch(valueType){
  case Int:{
    *this = value.i / f;
    break;
  }
  case Float:{
    *this = value.f / f;
    break;
  }
  case String:
  default:
    break;
  }

  return *this;
}
  
// Modulus operator
Value &Value::operator%=(const Value &v){
  if(!available||!v.available) return *this;
  if(v.valueType==Int){
    *this %= v.value.i;
  }
  return *this;
}

//  Modulus operator
Value &Value::operator%=(const int i){
  if(!available||infinity) return *this;
  if(valueType==Int){
    *this = value.i % i;
  }
  return *this;
}

// Addition operator
Value operator + (const Value &v1,const Value &v2){
  Value result=v1;
  return result+=v2;
}

// Subtraction operator
Value operator - (const Value &v1,const Value &v2){
  Value result=v1;
  return result-=v2;
}

// Multiplication operator
Value operator * (const Value &v1,const Value &v2){
  Value result=v1;
  return result*=v2;
}

// Division operator
Value operator / (const Value &v1,const Value &v2){ 
  Value result=v1;
  return result/=v2;
}

// Modulus operator
Value operator % (const Value &v1,const Value &v2){
  Value result=v1;
  return result%=v2;
}
 
// Addition operator  
Value operator + (const Value &v,const int i){
  Value result=v;
  return result+=i;
}

// Subtraction operator
Value operator - (const Value &v,const int i){
  Value result=v;
  return result-=i;
}
 
// Multiplication operator
Value operator * (const Value &v,const int i){
  Value result=v;
  return result*=i;
}

// Division operator
Value operator / (const Value &v,const int i){
  Value result=v;
  return result/=i;
}
 
// Modulus operator
Value operator % (const Value &v,const int i){
  Value result=v;
  return result%=i;
}

// Addition operator  
Value operator + (const Value &v,const float f){
  Value result=v;
  return result+=f;
}
 
// Subtraction operator
Value operator - (const Value &v,const float f){
  Value result=v;
  return result-=f;
}
 
// Multiplication operator
Value operator * (const Value &v,const float f){
  Value result=v;
  return result*=f;
}

// Division operator
Value operator / (const Value &v,const float f){
  Value result=v;
  return result/=f;
}

// Addition operator 
Value operator + (const Value &v,const char *str){
  Value result=v;
  return result+=str;
} 

// Comparison operator between Value and string.
int strcmp(const Value &v,const char *s){
  const char *tmp_str=v.getString();
  
  if(tmp_str==s) //These either point to the same string or are both 0
    return 0;
  if(!tmp_str) // This one is 0
    return -1;
  if(!s) // This one is 0
    return 1;
  return strcmp(tmp_str,s);
}

// Comparison operator between string and Value.
int strcmp(const char *s,const Value &v){
  const char *tmp_str=v.getString();

  if(tmp_str==s) //These either point to the same string or are both 0
    return 0;
  if(!tmp_str) // This one is 0
    return 1;
  if(!s) // This one is 0
    return -1;
  return strcmp(s,tmp_str);
}

// Output operator for Value class
ostream &operator << (ostream &o,const Value &v){
  if(v.available){
    switch(v.valueType){
    case Value::Int:{
      o << v.value.i;
      break;
    }
    case Value::Float:{
      if(v.infinity){
	if(v.value.f>0)
	  o << HUGE_VALF;
	else
	  o << -HUGE_VALF;
      }
      else{
	if(fabsf(v.value.f)>FLT_MIN)
	  o << v.value.f;
	else{
	  if(v.value.f>0) o << FLT_MIN;
	  else o << -FLT_MIN;
	}
      }
      break;
    }
    case Value::String:{
      if(v.value.s==0)
	o << "";
      else
	o  << "'" << v.value.s << "'"; 
    }
    }
  }
  
  return o;
}

// Output operator for a pointer to Value class object
ostream &operator << (ostream &o, const Value *v){
  if(v)
    o << *v;
  else
    o << "(nil)";
  return o;
}

// Input operator for Value class
istream &operator >> (istream &i,Value &v){
  char tok[MAX_STR];
  char c;
  int n=0;

  // Skip over white spaces first
  while(((c=i.peek())==' ')||(c=='\t')) i >> c;
  // Read in a string
  while((i >> c)&&(c!='\n')&&(c!='\t')&&(c!=' ')&&(n<MAX_STR))
    tok[n++]=c;

  if(n==0){  // Something went wrong
    // Better error checking should be implemented
    v.available = false;
  }else{    // Looks like a string has been read in
    tok[n]='\0';     // Properly terminate our string
    v.parse(tok);
  }

  return i;
}
	
// Input operator for a pointer to a Value class object
istream &operator >> (istream &i, Value *v){
  v = new Value();

  i >> *v;

  return i;
}


