/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppObsPool
 *
 * Purpose:    Buffer with observations
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iomanip>
#include "pppObsPool.h"
#include "pppClient.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::t_epoch::t_epoch(const bncTime& epoTime, vector<t_pppSatObs*>& obsVector,
                               bool pseudoObsIono) {
  _epoTime        = epoTime;
  _pseudoObsIono  = pseudoObsIono;
  for (unsigned ii = 0; ii < obsVector.size(); ii++) {
    _obsVector.push_back(obsVector[ii]);
  }
  obsVector.clear();
}

// Destructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::t_epoch::~t_epoch() {
  for (unsigned ii = 0; ii < _obsVector.size(); ii++) {
    delete _obsVector[ii];
  }
}

// Constructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::t_pppObsPool() {
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    _satCodeBiases[ii] = 0;
  }
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    _satPhaseBiases[ii] = 0;
  }
  _vTec = 0;
}

// Destructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::~t_pppObsPool() {
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    delete _satCodeBiases[ii];
  }
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    delete _satPhaseBiases[ii];
  }
  delete _vTec;
  while (_epochs.size() > 0) {
    delete _epochs.front();
    _epochs.pop_front();
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putCodeBias(t_satCodeBias* satCodeBias) {
  int iPrn = satCodeBias->_prn.toInt();
  delete _satCodeBiases[iPrn];
  _satCodeBiases[iPrn] = satCodeBias;
  if (OPT->_logMode > t_pppOptions::normal) {
    LOG << "codeBias " << string(satCodeBias->_time) << ' ' << satCodeBias->_prn.toString();
    for (const auto& bias : satCodeBias->_bias) {
      LOG << "  " << bias._rnxType2ch << ' ' << setw(7) << setprecision(3) << bias._value;
    }
    LOG << endl;
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putPhaseBias(t_satPhaseBias* satPhaseBias) {
  int iPrn = satPhaseBias->_prn.toInt();
  delete _satPhaseBiases[iPrn];
  _satPhaseBiases[iPrn] = satPhaseBias;
  if (OPT->_logMode > t_pppOptions::normal) {
    LOG.setf(ios::fixed);
    LOG << "phaseBias " << string(satPhaseBias->_time) << ' ' << satPhaseBias->_prn.toString()
        << ' ' << setw(7) << setprecision(2) << satPhaseBias->_yaw     * 180.0 / M_PI
        << ' ' << setw(7) << setprecision(3) << satPhaseBias->_yawRate * 180.0 / M_PI;
    for (const auto& bias : satPhaseBias->_bias) {
      LOG << "  " << bias._rnxType2ch << ' ' << setw(2) << bias._jumpCounter;
      if (bias._fixIndicator) {
        LOG << " x ";
      }
      else {
        LOG << " . ";
      }
      LOG << setw(6) << setprecision(3) << bias._value;
    }
    LOG << endl;
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::clearCodeBiases(char sys) {
  for (unsigned iPrn = 1; iPrn <= t_prn::MAXPRN; ++iPrn) {
    t_prn prn; prn.set(iPrn);
    if (prn.system() == sys) {
      delete _satCodeBiases[iPrn];
      _satCodeBiases[iPrn] = 0;
    }
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::clearPhaseBiases(char sys) {
  for (unsigned iPrn = 1; iPrn <= t_prn::MAXPRN; ++iPrn) {
    t_prn prn; prn.set(iPrn);
    if (prn.system() == sys) {
      delete _satPhaseBiases[iPrn];
      _satPhaseBiases[iPrn] = 0;
    }
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putTec(t_vTec* vTec) {
  delete _vTec;
  _vTec = new t_vTec(*vTec);
  delete vTec;
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putEpoch(const bncTime& epoTime, vector<t_pppSatObs*>& obsVector,
                            bool pseudoObsIono) {
  const unsigned MAXSIZE = 2;
  _epochs.push_back(new t_epoch(epoTime, obsVector, pseudoObsIono));

  if (_epochs.size() > MAXSIZE) {
    delete _epochs.front();
    _epochs.pop_front();
  }
}
