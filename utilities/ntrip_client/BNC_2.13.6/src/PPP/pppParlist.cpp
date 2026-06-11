/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppParlist
 *
 * Purpose:    List of estimated parameters
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <newmatio.h>

#include "pppParlist.h"
#include "pppSatObs.h"
#include "pppStation.h"
#include "bncutils.h"
#include "bncconst.h"
#include "pppClient.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppParam::t_pppParam(e_type type, const t_prn& prn, t_lc LC,
                       const vector<t_pppSatObs*>* obsVector) {

  _type     = type;
  _prn      = prn;
  _sys      = '?';
  _LC       = LC;
  _x0       = 0.0;
  _indexOld = -1;
  _indexNew = -1;
  _noise    = 0.0;
  _ambInfo = new t_ambInfo();

  switch (_type) {
  case crdX:
    _epoSpec = false;
    _sigma0  = OPT->_aprSigCrd[0];
    _noise   = OPT->_noiseCrd[0];
    break;
  case crdY:
    _epoSpec = false;
    _sigma0  = OPT->_aprSigCrd[1];
    _noise   = OPT->_noiseCrd[1];
    break;
  case crdZ:
    _epoSpec = false;
    _sigma0  = OPT->_aprSigCrd[2];
    _noise   = OPT->_noiseCrd[2];
    break;
  case rClk:
    _epoSpec = true;
    _sigma0  = OPT->_aprSigClk;
    break;
  case amb:
    _epoSpec = false;
    _sigma0  = OPT->_aprSigAmb;
    if (obsVector) {
      for (unsigned ii = 0; ii < obsVector->size(); ii++) {
        const t_pppSatObs* obs = obsVector->at(ii);
        if (obs->prn() == _prn) {
          _x0 = floor((obs->obsValue(LC) - obs->cmpValue(LC)) / obs->lambda(LC) + 0.5);
          break;
        }
      }
    }
    break;
  case trp:
    _epoSpec = false;
    _sigma0  = OPT->_aprSigTrp;
    _noise   = OPT->_noiseTrp;
    break;
  case ion:
    _epoSpec = true;
    _sigma0  = OPT->_aprSigIon;
    break;
  case bias:
    _epoSpec = true;
    _sigma0  = OPT->_aprSigCodeBias;
    break;
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_pppParam::~t_pppParam() {
  delete _ambInfo;
}
//
////////////////////////////////////////////////////////////////////////////
double t_pppParam::partial(const bncTime& /* epoTime */, const t_pppSatObs* obs,
                           const t_lc& obsLC) const {

  // Special Case - Melbourne-Wuebbena
  // ---------------------------------
  if (obsLC._type == t_lc::MW && _type != amb) {
    return 0.0;
  }

  // Special Case - GIM
  // ------------------
  if (obsLC._type == t_lc::GIM) {
    if (_type == ion) {
      return 1.0;
    }
    else {
      return 0.0;
    }
  }

  const t_pppStation* sta  = PPP_CLIENT->staRover();
  ColumnVector        rhoV = sta->xyzApr() - obs->xc().Rows(1,3);

  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  obs->lcCoeff(obsLC, codeCoeff, phaseCoeff, ionoCoeff);
  
  switch (_type) {
  case crdX:
    return (sta->xyzApr()[0] - obs->xc()[0]) / rhoV.NormFrobenius();
  case crdY:
    return (sta->xyzApr()[1] - obs->xc()[1]) / rhoV.NormFrobenius();
  case crdZ:
    return (sta->xyzApr()[2] - obs->xc()[2]) / rhoV.NormFrobenius();
  case rClk:
    return (_sys != obsLC.system() || obsLC.isGeometryFree()) ? 0.0 : 1.0;
  case amb:
    if (obs->prn() == _prn) {
      if      (obsLC == _LC) {
        return (obs->lambda(obsLC));
      }
      else if (_LC._type == t_lc::phase) {
        return obs->lambda(_LC) * phaseCoeff[_LC._frq1];
      }
      else if (obsLC._type == t_lc::phaseIF && _LC._type == t_lc::MW) {
        return obs->lambda(obsLC) * obs->lambda(_LC) / obs->lambda(t_lc(t_lc::phase, _LC._frq2));
      }
    }
    break;
  case trp:
    return 1.0 / sin(obs->eleSat());
  case ion:
    if (obs->prn() == _prn) {
      if (obsLC._type == t_lc::code || obsLC._type == t_lc::phase) {
        return ionoCoeff[obsLC._frq1];
      }
    }
    break;
  case bias:
    return (_LC == obsLC ? 1.0 : 0.0);
  }
  return 0.0;
}

//
////////////////////////////////////////////////////////////////////////////
string t_pppParam::toString() const {
  stringstream ss;
  switch (_type) {
  case crdX:
    ss << "CRD_X";
    break;
  case crdY:
    ss << "CRD_Y";
    break;
  case crdZ:
    ss << "CRD_Z";
    break;
  case rClk:
    ss << "REC_CLK  " << _sys << "  ";
    break;
  case trp:
    ss << "TRP         ";
    break;
  case amb:
    ss << "AMB  " << left << setw(3) << _LC.toString() << right << ' ' << _prn.toString();
    break;
  case ion:
    ss << "ION  " << left << setw(3) << _LC.toString() << right << ' ' << _prn.toString();
    break;
  case bias:
    char sys = t_frequency::toSystem(_LC._frq1);
    ss << "BIA  " << left << setw(3) << _LC.toString() << right << ' ' << sys << "  ";
    break;
  }
  return ss.str();
}

// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppParlist::t_pppParlist() {
}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_pppParlist::~t_pppParlist() {

  for (unsigned ii = 0; ii < _params.size(); ii++) {
    delete _params[ii];
  }
}

//
////////////////////////////////////////////////////////////////////////////
t_irc t_pppParlist::set(const bncTime& epoTime, const std::vector<t_pppSatObs*>& obsVector) {

  // Remove some Parameters
  // ----------------------
  vector<t_pppParam*>::iterator it = _params.begin();
  while (it != _params.end()) {
    t_pppParam* par = *it;

    bool remove = false;

    if      (par->epoSpec()) {
      remove = true;
    }

    else if (par->type() == t_pppParam::amb  ||
             par->type() == t_pppParam::crdX ||
             par->type() == t_pppParam::crdY ||
             par->type() == t_pppParam::crdZ ||
             par->type() == t_pppParam::ion  ) {
      if (par->lastObsTime().valid() && (epoTime - par->lastObsTime() > 60.0)) {
        remove = true;
      }
    }

    if (remove) {
#ifdef BNC_DEBUG_PPP
//      LOG << "remove0 " << par->toString() << std::endl;
#endif
      delete par;
      it = _params.erase(it);
    }
    else {
      ++it;
    }
  }

  // Check which systems have observations
  // -------------------------------------
  vector<char> systems = OPT->systems();
  for (char sys : systems) {
    _usedSystems[sys] = 0;
    for (unsigned jj = 0; jj < obsVector.size(); jj++) {
      const t_pppSatObs* satObs = obsVector[jj];
      if (satObs->prn().system() == sys) {
        _usedSystems[sys]++;
      }
    }
  };

  // Check whether parameters have observations
  // ------------------------------------------
  for (unsigned ii = 0; ii < _params.size(); ii++) {
    t_pppParam* par = _params[ii];
    if (par->prn() == 0) {
      par->setLastObsTime(epoTime);
      if (par->firstObsTime().undef()) {
        par->setFirstObsTime(epoTime);
      }
    }
    else {
      for (unsigned jj = 0; jj < obsVector.size(); jj++) {
        const t_pppSatObs* satObs = obsVector[jj];
        if (satObs->prn() == par->prn()) {
          par->setLastObsTime(epoTime);
          if (par->firstObsTime().undef()) {
            par->setFirstObsTime(epoTime);
          }
          break;
        }
      }
    }
  }

  // Required Set of Parameters
  // --------------------------
  vector<t_pppParam*> required;

  // Coordinates
  // -----------
  required.push_back(new t_pppParam(t_pppParam::crdX, t_prn()));
  required.push_back(new t_pppParam(t_pppParam::crdY, t_prn()));
  required.push_back(new t_pppParam(t_pppParam::crdZ, t_prn()));

  // Receiver Clocks
  // ---------------
  for (const auto& [sys, numObs] : _usedSystems) {
    if (numObs > 0) {
      t_pppParam* clk = new t_pppParam(t_pppParam::rClk, t_prn());
      clk->setSystem(sys);
      required.push_back(clk);
    }
  }

  // Troposphere
  // -----------
  if (OPT->estTrp()) {
    required.push_back(new t_pppParam(t_pppParam::trp, t_prn()));
  }

  // Ionosphere
  // ----------
  if (OPT->_ionoModelType == OPT->est) {
    for (unsigned jj = 0; jj < obsVector.size(); jj++) {
      const t_pppSatObs* satObs = obsVector[jj];
      std::vector<t_lc> LCs = OPT->LCs(satObs->prn().system());
      for (auto it = LCs.begin(); it != LCs.end(); ++it) {
        const t_lc& lc = *it;
        if (!lc.isIonoFree()) {
          required.push_back(new t_pppParam(t_pppParam::ion, satObs->prn()));
          break;
        }
      }
    }
  }
  
  // Ambiguities
  // -----------
  for (unsigned jj = 0; jj < obsVector.size(); jj++) {
    const t_pppSatObs*  satObs = obsVector[jj];
    char sys = satObs->prn().system();
    const vector<t_lc>& ambLCs = OPT->ambLCs(sys);
    for (unsigned ii = 0; ii < ambLCs.size(); ii++) {
      if (ambLCs[ii]._frq1 == t_frequency::G5 && !satObs->isValid(ambLCs[ii])) {
        continue;
      }
      required.push_back(new t_pppParam(t_pppParam::amb, satObs->prn(), ambLCs[ii], &obsVector));
    }
  }

  // Biases
  // ------
  int maxSkip = (OPT->_pseudoObsIono ? 1 : 2);
  for (const auto& [sys, numObs] : _usedSystems) {
    if (numObs > 0) {
      bool ar   = OPT->arSystem(sys);
      int  skip = 0;
      vector<t_lc> LCs = OPT->LCs(sys);
      for (const t_lc& lc : LCs) {
        if (ar) {
          if (skip < maxSkip && lc.includesPhase()) {
            skip += 1;
          }
          else {
            required.push_back(new t_pppParam(t_pppParam::bias, t_prn(), lc));
          }
        }
        else {
          if (lc.includesCode()) {
            if (skip < maxSkip) {
              skip += 1;
            }
            else {
              required.push_back(new t_pppParam(t_pppParam::bias, t_prn(), lc));
            }
          }
        }
      }
    }
  }

  // Check if all required parameters are present
  // --------------------------------------------
  for (unsigned ii = 0; ii < required.size(); ii++) {
    t_pppParam* parReq = required[ii];

    bool found = false;
    for (unsigned jj = 0; jj < _params.size(); jj++) {
      t_pppParam* parOld = _params[jj];
      if (parOld->isEqual(parReq)) {
        found = true;
        break;
      }
    }
    if (found) {
      delete parReq;
    }
    else {
      _params.push_back(parReq);
    }
  }

  // Set Parameter Indices
  // ---------------------
  sort(_params.begin(), _params.end(), t_pppParam::sortFunction);

  for (unsigned ii = 0; ii < _params.size(); ii++) {
    t_pppParam* par = _params[ii];
    par->setIndex(ii);
    for (unsigned jj = 0; jj < obsVector.size(); jj++) {
      const t_pppSatObs* satObs = obsVector[jj];
      if (satObs->prn() == par->prn()) {
        par->setAmbEleSat(satObs->eleSat());
        par->stepAmbNumEpo();
      }
    }
  }

  return success;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppParlist::printResult(const bncTime& epoTime, const SymmetricMatrix& QQ,
                               const ColumnVector& xx, double fixRatio) const {

  string epoTimeStr = string(epoTime);
  const t_pppStation* sta = PPP_CLIENT->staRover();

  LOG << endl;

  t_pppParam* parX = 0;
  t_pppParam* parY = 0;
  t_pppParam* parZ = 0;
  for (unsigned ii = 0; ii < _params.size(); ii++) {
    t_pppParam* par = _params[ii];
    if      (par->type() == t_pppParam::crdX) {
      parX = par;
    }
    else if (par->type() == t_pppParam::crdY) {
      parY = par;
    }
    else if (par->type() == t_pppParam::crdZ) {
      parZ = par;
    }
    else {
      int ind = par->indexNew();
      double apr = (par->type() == t_pppParam::trp) ?
        t_tropo::delay_saast(sta->xyzApr(), M_PI/2.0) :  par->x0();
      LOG << epoTimeStr << ' ' << par->toString() << ' '
          << setw(10) << setprecision(4) << apr << ' '
          << showpos << setw(10) << setprecision(4) << xx[ind] << noshowpos << " +- "
          << setw(8)  << setprecision(4) << sqrt(QQ[ind][ind]);
      if (par->type() == t_pppParam::amb) {
        LOG << " el = " << setw(6) << setprecision(2) << par->ambEleSat() * 180.0 / M_PI
            << " epo = " << setw(4) << par->ambNumEpo();
      }
      LOG << endl;
    }
  }

  if (parX && parY && parZ) {

	ColumnVector xyz(3);
    xyz[0] = xx[parX->indexNew()];
    xyz[1] = xx[parY->indexNew()];
    xyz[2] = xx[parZ->indexNew()];

    ColumnVector neu(3);
    xyz2neu(sta->ellApr().data(), xyz.data(), neu.data());

    SymmetricMatrix QQxyz = QQ.SymSubMatrix(1,3);

    SymmetricMatrix QQneu(3);
    covariXYZ_NEU(QQxyz, sta->ellApr().data(), QQneu);

    LOG << epoTimeStr << ' ' << sta->name()
        << " X = " << setprecision(4) << sta->xyzApr()[0] + xyz[0] << " +- "
        << setprecision(4) << sqrt(QQxyz[0][0])

        << " Y = " << setprecision(4) << sta->xyzApr()[1] + xyz[1] << " +- "
        << setprecision(4) << sqrt(QQxyz[1][1])

        << " Z = " << setprecision(4) << sta->xyzApr()[2] + xyz[2] << " +- "
        << setprecision(4) << sqrt(QQxyz[2][2])

        << " dN = " << setprecision(4) << neu[0] << " +- "
        << setprecision(4) << sqrt(QQneu[0][0])

        << " dE = " << setprecision(4) << neu[1] << " +- "
        << setprecision(4) << sqrt(QQneu[1][1])

        << " dU = " << setprecision(4) << neu[2] << " +- "
        << setprecision(4) << sqrt(QQneu[2][2]);

    if (fixRatio > 0.0) {
      LOG << " fix " <<  int(100*fixRatio) << " %";
    }
    else {
      LOG << " flt ";
    }

    LOG << endl;
  }
  return;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppParlist::printParams(const bncTime& epoTime) {

  for (unsigned iPar = 0; iPar < _params.size(); iPar++) {
    LOG << _params[iPar]->toString()
        << "\t  lastObsTime().valid() \t" << _params[iPar]->lastObsTime().valid()
        << "\t  epoTime - par->lastObsTime() \t" << (epoTime - _params[iPar]->lastObsTime())
        << endl;
  }
}

