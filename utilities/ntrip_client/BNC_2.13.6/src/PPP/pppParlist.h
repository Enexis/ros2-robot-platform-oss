#ifndef PARLIST_H
#define PARLIST_H

#include <vector>
#include <string>
#include "pppInclude.h"
#include "t_prn.h"
#include "bnctime.h"

namespace BNC_PPP {

class t_pppSatObs;

class t_pppParam {
 public:
  enum e_type {crdX, crdY, crdZ, rClk, trp, ion, amb, bias};

  t_pppParam(e_type type, const t_prn& prn, t_lc LC = t_lc(), const std::vector<t_pppSatObs*>* obsVector = 0);
  ~t_pppParam();

  e_type type() const {return _type;}
  double x0()  const {return _x0;}
  double partial(const bncTime& epoTime, const t_pppSatObs* obs, const t_lc& LC) const;
  bool   epoSpec() const {return _epoSpec;}
  bool   isEqual(const t_pppParam* par2) const {
    return (_type == par2->_type && _prn == par2->_prn && _LC == par2->_LC && _sys == par2->_sys);
  }
  void   setIndex(int indexNew) {
    _indexOld = _indexNew;
    _indexNew = indexNew;
  }
  void resetIndex() {
    _indexOld = -1;
  }
  int         indexOld() const {return _indexOld;}
  int         indexNew() const {return _indexNew;}
  double      sigma0() const {return _sigma0;}
  double      noise() const {return _noise;}
  t_lc        LC() const {return _LC;}
  t_prn       prn() const {return _prn;}
  std::string toString() const;

  const bncTime& lastObsTime() const {return _lastObsTime;}
  void setLastObsTime(const bncTime& epoTime) {_lastObsTime = epoTime;}
  const bncTime& firstObsTime() const {return _firstObsTime;}
  void setFirstObsTime(const bncTime& epoTime) {_firstObsTime = epoTime;}

  bool     ambResetCandidate() const   {return _ambInfo && _ambInfo->_resetCandidate;}
  void     setAmbResetCandidate()      {if (_ambInfo) _ambInfo->_resetCandidate = true;}
  double   ambEleSat() const           {return _ambInfo ? _ambInfo->_eleSat : 0.0;}
  void     setAmbEleSat(double eleSat) {if (_ambInfo) _ambInfo->_eleSat = eleSat;}
  unsigned ambNumEpo() const           {return _ambInfo ? _ambInfo->_numEpo : 0;}
  void     stepAmbNumEpo()             {if (_ambInfo) _ambInfo->_numEpo += 1;}
  char     system() const {
    if (_prn.valid()) {
      return _prn.system();
    }
    else if (_LC.valid()) {
      return _LC.system();
    }
    else {
      return _sys;
    }
  }
  void     setSystem(char sys) {_sys = sys;}

  static bool sortFunction(const t_pppParam* p1, const t_pppParam* p2) {
    if      (p1->_type != p2->_type) {
      return p1->_type < p2->_type;
    }
    else if (p1->_LC != p2->_LC) {
      return p1->_LC < p2->_LC;
    }
    else if (p1->_prn != p2->_prn) {
      return p1->_prn < p2->_prn;
    }
    return false;
  }

 private:
  class t_ambInfo {
   public:
    t_ambInfo() {
      _resetCandidate = false;
      _eleSat         = 0.0;
      _numEpo         = 0;
    }
    ~t_ambInfo() {}
    bool     _resetCandidate;
    double   _eleSat;
    unsigned _numEpo;
  };
  e_type       _type;
  t_prn        _prn;
  char         _sys;
  t_lc         _LC;
  double       _x0;
  bool         _epoSpec;
  int          _indexOld;
  int          _indexNew;
  double       _sigma0;
  double       _noise;
  t_ambInfo*   _ambInfo;
  bncTime      _lastObsTime;
  bncTime      _firstObsTime;
};

class t_pppParlist {
 public:
  t_pppParlist();
  ~t_pppParlist();
  t_irc set(const bncTime& epoTime, const std::vector<t_pppSatObs*>& obsVector);
  unsigned nPar() const {return _params.size();}
  const std::vector<t_pppParam*>& params() const {return _params;}
  std::vector<t_pppParam*>& params() {return _params;}
  void printResult(const bncTime& epoTime, const SymmetricMatrix& QQ,
                   const ColumnVector& xx, double fixRatio = 0.0) const;
  void printParams(const bncTime& epoTime);

 private:
  std::vector<t_pppParam*> _params;
  std::map<char, int>      _usedSystems;
};

}

#endif
