#include <sstream>
#include <iomanip>
#include <stdlib.h>

#include "t_prn.h"

using namespace std;

// Set from number
//////////////////////////////////////////////////////////////////////////////
void t_prn::set(unsigned nn) {
  const static unsigned maxGPS     = MAXPRN_GPS;
  const static unsigned maxGLONASS = MAXPRN_GPS + MAXPRN_GLONASS;
  const static unsigned maxGALILEO = MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO;
  const static unsigned maxQZSS    = MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + MAXPRN_QZSS;
  const static unsigned maxSBAS    = MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + MAXPRN_QZSS + MAXPRN_SBAS;
  const static unsigned maxBDS     = MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + MAXPRN_QZSS + MAXPRN_SBAS + MAXPRN_BDS;
  const static unsigned maxNavIC   = MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + MAXPRN_QZSS + MAXPRN_SBAS + MAXPRN_BDS + MAXPRN_NavIC;
  _system = 'G';
  _number = 0;
  _flag   = 0;
  if      (nn <= maxGPS) {
    _system = 'G';
    _number = nn;
  }
  else if (nn <= maxGLONASS) {
    _system = 'R';
    _number = nn - maxGPS;
  }
  else if (nn <= maxGALILEO) {
    _system = 'E';
    _number = nn - maxGLONASS;
  }
  else if (nn <= maxQZSS) {
    _system = 'J';
    _number = nn - maxGALILEO;
  }
  else if (nn <= maxSBAS) {
    _system = 'S';
    _number = nn - maxQZSS;
  }
  else if (nn <= maxBDS) {
    _system = 'C';
    _number = nn - maxSBAS;
  }
  else if (nn <= maxNavIC) {
    _system = 'I';
    _number = nn - maxBDS;
  }
} 

//
//////////////////////////////////////////////////////////////////////////////
int t_prn::toInt() const {
  if      (_system == 'G') {
    return _number;
  }
  else if (_system == 'R') {
    return MAXPRN_GPS + _number;
  }
  else if (_system == 'E') {
    return MAXPRN_GPS + MAXPRN_GLONASS + _number;
  }
  else if (_system == 'J') {
	  return MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + _number;
  }
  else if (_system == 'S') {
	  return MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + MAXPRN_QZSS + _number;
  }
  else if (_system == 'C') {
  	return MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + MAXPRN_QZSS + MAXPRN_SBAS + _number;
  }
  else if (_system == 'I') {
    return MAXPRN_GPS + MAXPRN_GLONASS + MAXPRN_GALILEO + MAXPRN_QZSS + MAXPRN_SBAS + MAXPRN_BDS + _number;
  }
  return 0;
}

//
//////////////////////////////////////////////////////////////////////////////
string t_prn::toString() const {
  stringstream ss;
  ss << _system << setfill('0') << setw(2) << _number;
  return ss.str();
}

//
//////////////////////////////////////////////////////////////////////////////
string t_prn::toInternalString() const {
  stringstream ss;
  ss << _system << setfill('0') << setw(2) << _number << '_' << _flag;
  return ss.str();
}

// Set from string
////////////////////////////////////////////////////////////////////////////
void t_prn::set(const std::string& str) {
  unsigned    prn    = 0;
  char        system = '\x0';
  const char* number = 0;
  if ( str[0] == 'G' || str[0] == 'R' || str[0] == 'E' ||
       str[0] == 'J' || str[0] == 'S' || str[0] == 'C' ||
       str[0] == 'I') {
    system = str[0];
    number = str.c_str() + 1;
  }
  else if ( isdigit(str[0]) ) {
    system = 'G';
    number = str.c_str();
  }
  else {
    throw "t_prn::set: wrong satellite ID: " + str;
  }

  char* tmpc = 0;
  prn = strtol(number, &tmpc, 10);
  if ( tmpc == number || *tmpc != '\x0' ) {
    throw "t_prn::set: wrong satellite ID: " + str;
  }

  try {
    this->set(system, prn);
  }
  catch (string exc) {
    throw "t_prn::set: wrong satellite ID: " + str;
  }
}

//
//////////////////////////////////////////////////////////////////////////////
t_prn::operator unsigned() const {
  return toInt();
}

//
//////////////////////////////////////////////////////////////////////////////
istream& operator >> (istream& in, t_prn& prn) {
  string str;
  in >> str;
  if (str.length() == 1 && !isdigit(str[0])) {
    string str2;
    in >> str2;
    str += str2;
  }
  prn.set(str);
  return in;
}
