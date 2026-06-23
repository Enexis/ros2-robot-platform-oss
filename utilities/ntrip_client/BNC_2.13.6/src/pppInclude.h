#ifndef PPP_H
#define PPP_H

#include <string>
#include <vector>
#include <newmat.h>
#include <sstream>

#include "bncconst.h"
#include "bnctime.h"
#include "ephemeris.h"
#include "t_prn.h"
#include "satObs.h"

namespace BNC_PPP {

const double ZEROVALUE = 1e-100;

class t_except {
 public:
  t_except(const char* msg) {
    _msg = msg;
  }
  ~t_except() {}
  std::string what() {return _msg;}
 private:
  std::string _msg;
};

class t_output {
 public:
  bncTime      _epoTime;
  double       _xyzRover[3];
  double       _covMatrix[6];
  double       _neu[3];
  double       _trp0;
  double       _trp;
  double       _trpStdev;
  int          _numSat;
  double       _hDop;
  std::string  _log;
  double       _fixRatio;
  bool         _error;
};

class t_lc {
public:
  enum type {dummy = 0, code, phase, codeIF, phaseIF,  MW, CL, GIM, maxLc};

  t_lc() : _type(dummy), _frq1(t_frequency::dummy), _frq2(t_frequency::dummy) {}

  t_lc(type tt, t_frequency::type frq1, t_frequency::type frq2 = t_frequency::dummy)
    : _type(tt), _frq1(frq1), _frq2(frq2) {
  }

  bool valid() const {
    if      (_type == dummy) {
      return false;
    }
    else if (_type == GIM) {
      return true;
    }
    else {
      if (_frq1 == t_frequency::dummy) {
        return false;
      }
      if (t_lc::needs2ndFrq(_type) && _frq2 == t_frequency::dummy) {
        return false;
      }
      return true;
    }
  }

  char system() const {
    return t_frequency::toSystem(_frq1);
  }

  static bool needs2ndFrq(type tt) {
    if (tt == codeIF || tt == phaseIF || tt == MW) {
      return true;
    }
    else {
      return false;
    }
  }

  bool includesPhase() const {
    if (_type == phase || _type == phaseIF || _type == MW || _type == CL) {
      return true;
    }
    else {
      return false;
    }
  }

  bool includesCode() const {
    if (_type == code || _type == codeIF || _type == MW || _type == CL) {
      return true;
    }
    else {
      return false;
    }
  }

  bool isIonoFree() const {
    if (_type == codeIF || _type == phaseIF || _type == MW || _type == CL) {
      return true;
    }
    else {
      return false;
    }
  }

  bool isGeometryFree() const {
    if (_type == MW || _type == CL || _type == GIM) {
      return true;
    }
    else {
      return false;
    }
  }

  t_frequency::type toFreq() const {
    if (_frq2 != t_frequency::dummy) {
      return t_frequency::dummy;
    }
    else {
      return _frq1;
    }
  }

  std::string toString() const {
    std::stringstream out;
    if      (_type == code) {
      out << 'c' << t_frequency::toString(_frq1);
    }
    else if (_type == phase) {
      out << 'l' << t_frequency::toString(_frq1);
    }
    else if (_type == codeIF) {
      out << 'c' << t_frequency::toSystem(_frq1) << "IF" ;
    }
    else if (_type == phaseIF) {
      out << 'l' << t_frequency::toSystem(_frq1) << "IF" ;
    }
    else if (_type == MW) {
      out << t_frequency::toSystem(_frq1) << "MW" ;
    }
    else if (_type == CL) {
      out << t_frequency::toSystem(_frq1) << "CL" ;
    }
    else if (_type == GIM) {
      out << t_frequency::toSystem(_frq1) << "GIM" ;
    }
    return out.str();
  }

  bool operator==(const t_lc& other) const {
    if (_type == other._type && _frq1 == other._frq1 && _frq2 == other._frq2) {
      return true;
    }
    else {
      return false;
    }
  }

  bool operator!=(const t_lc& other) const {
    return !(*this == other);
  }

  bool operator <(const t_lc& other) const {
    if      (_type != other._type) {
      return _type < other._type;
    }
    else if (_frq1 != other._frq1) {
      return _frq1 < other._frq1;
    }
    else {
      return _frq2 < other._frq2;
    }
  }

  type              _type;
  t_frequency::type _frq1;
  t_frequency::type _frq2;
};

class interface_pppClient {
 public:
  virtual      ~interface_pppClient() {};
  virtual void processEpoch(const std::vector<t_satObs*>& satObs, t_output* output) = 0;
  virtual void putEphemeris(const t_eph* eph) = 0;
  virtual void putOrbCorrections(const std::vector<t_orbCorr*>& corr) = 0;
  virtual void putClkCorrections(const std::vector<t_clkCorr*>& corr) = 0;
  virtual void putCodeBiases(const std::vector<t_satCodeBias*>& satCodeBias) = 0;
};

} // namespace BNC_PPP

#endif
