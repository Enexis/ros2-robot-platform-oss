#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <vector>
#include <set>
#include <newmat.h>
#include "pppInclude.h"

namespace BNC_PPP {

class t_pppOptions {
 public:
  enum iono_type {est};
  enum e_logMode {normal, debug, all};

  class SysTrkModes {
  public:
    class FrqTrkModes {
    public:
      FrqTrkModes(t_frequency::type frq, const std::string trkModes)
        : _frq(frq), _trkModes(trkModes)  {}
      t_frequency::type _frq;
      std::string       _trkModes;
    };
    std::vector<FrqTrkModes> _frqTrkModes;
  };

  class ArOptions {
  public:
    std::set<char> _systems;
    double         _minEle    = 0.0;
    unsigned       _minNumEpo = 0;
    int            _minNumSat = 0;
    bool           _useYaw    = false;
    double         _maxFrac   = 0.0;
    double         _maxSig    = 0.0;
  };
  
  t_pppOptions();
  ~t_pppOptions();

  std::vector<char>        systems() const;
  const std::vector<t_lc>& LCs(char system) const;
  std::vector<t_lc>        ambLCs(char system) const;
  bool                     useSystem(char system) const {return LCs(system).size() > 0;}
  bool                     useOrbClkCorr() const;
  bool                     estTrp() const {return _aprSigTrp > 0.0 || _noiseTrp > 0.0;}
  bool xyzAprRoverSet() const {
    return (_xyzAprRover[0] != 0.0 || _xyzAprRover[1] != 0.0 || _xyzAprRover[2] != 0.0);
  }
  void setTrkModes(const std::string& trkStr);
  const SysTrkModes* sysTrkModes(char sys) const {
    auto it = _trkModesMap.find(sys);
    return (it != _trkModesMap.end() ? &it->second : 0);
  }
  void setLCs(char sys, const std::string& lcStr);
  void defaultFrqs(char sys, t_frequency::type& frq1, t_frequency::type& frq2) const;
  bool ambRes() const {return _ar._systems.size() > 0;}
  bool arSystem(char sys) const {return _ar._systems.find(sys) != _ar._systems.end();}

  iono_type                     _ionoModelType;
  bool                          _realTime;
  std::string                   _crdFile;
  std::string                   _corrMount;
  std::string                   _biasMount;
  std::string                   _ionoMount;
  bool                          _isAPC;
  std::string                   _rinexObs;
  std::string                   _rinexNav;
  std::string                   _corrFile;
  std::string                   _biasFile;
  std::string                   _ionoFile;
  double                        _corrWaitTime;
  std::string                   _roverName;
  ColumnVector                  _xyzAprRover;
  ColumnVector                  _neuEccRover;
  std::string                   _recNameRover;
  std::string                   _antNameRover;
  std::string                   _antexFileName;
  std::string                   _blqFileName;
  double                        _sigmaC1;
  double                        _sigmaL1;
  double                        _sigmaGIM;
  double                        _maxResC1;
  double                        _maxResL1;
  double                        _maxResGIM;
  bool                          _eleWgtCode;
  bool                          _eleWgtPhase;
  double                        _minEle;
  int                           _minObs;
  ColumnVector                  _aprSigCrd;
  double                        _aprSigClk;
  double                        _aprSigTrp;
  double                        _aprSigIon;
  double                        _aprSigAmb;
  double                        _aprSigCodeBias;
  ColumnVector                  _noiseCrd;
  double                        _noiseTrp;
  int                           _nmeaPort;
  double                        _seedingTime;
  std::vector<t_lc>             _LCsGPS;
  std::vector<t_lc>             _LCsGLONASS;
  std::vector<t_lc>             _LCsGalileo;
  std::vector<t_lc>             _LCsBDS;
  bool                          _pseudoObsIono;
  bool                          _refSatRequired;
  ArOptions                     _ar;
  e_logMode                     _logMode;
private:  
  std::map<char, SysTrkModes>   _trkModesMap;
};

}

#endif
