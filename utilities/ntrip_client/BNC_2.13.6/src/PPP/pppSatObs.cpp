/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppSatObs
 *
 * Purpose:    Satellite observations
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/


#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <set>
#include <newmatio.h>

#include "pppSatObs.h"
#include "bncconst.h"
#include "pppEphPool.h"
#include "pppStation.h"
#include "bncutils.h"
#include "bncantex.h"
#include "pppObsPool.h"
#include "pppClient.h"

using namespace BNC_PPP;
using namespace std;


// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppSatObs::t_pppSatObs(const t_satObs& pppSatObs) {
  _prn        = pppSatObs._prn;
  _time       = pppSatObs._time;
  _outlier    = false;
  _valid      = true;
  _reference  = false;
  _stecSat    = 0.0;
  for (unsigned ii = 0; ii < t_frequency::max; ii++) {
    _obs[ii] = 0;
  }
  prepareObs(pppSatObs);
}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_pppSatObs::~t_pppSatObs() {
  for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
    delete _obs[iFreq];
  }
}

//
////////////////////////////////////////////////////////////////////////////
bool t_pppSatObs::isBetter(const t_frqObs* aa, t_frqObs* bb, const string& trkModes) const {

  if (!trkModes.empty()) {
    size_t posA = trkModes.find(aa->trkChar());
    size_t posB = trkModes.find(bb->trkChar());
    if (posA != posB) {
      if      (posA == string::npos) {
        return false;
      }
      else if (posB == string::npos) {
        return true;
      }
      else {
        return posA < posB;
      }
    }
  }
      
  unsigned numValA = 0;
  if (aa->_codeValid)  numValA += 1;
  if (aa->_phaseValid) numValA += 1;

  unsigned numValB = 0;
  if (bb->_codeValid)  numValB += 1;
  if (bb->_phaseValid) numValB += 1;

  if (numValA != numValB) {
    return numValA > numValB;
  }

  return false;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::prepareObs(const t_satObs& satObs) {

  _model.reset();

  const t_pppOptions::SysTrkModes* sysTrkModes = OPT->sysTrkModes(_prn.system());
  using FrqTrkModes = t_pppOptions::SysTrkModes::FrqTrkModes;
  
  for (const t_frqObs* obs : satObs._obs) {
    if ( (obs->_codeValid || obs->_phaseValid)) {
      t_frequency::type frq = t_frequency::toFreq(_prn.system(), obs->frqChar());
      if (frq == t_frequency::dummy) {
        continue;
      }
      string trkModes;
      if (sysTrkModes) {
        const vector<FrqTrkModes>& frqTrkModes = sysTrkModes->_frqTrkModes;
        auto it = find_if(frqTrkModes.begin(), frqTrkModes.end(),
                          [frq](const FrqTrkModes& mm){return mm._frq == frq;});
        if (it != frqTrkModes.end()) {
          trkModes = it->_trkModes;
        }
      }
      if (_obs[frq] == 0 || isBetter(obs, _obs[frq], trkModes)) {
        delete _obs[frq];  
        _obs[frq] = new t_frqObs(*obs);
      }
    }
  }
  
  // Check whether all required frequencies available
  // ------------------------------------------------
  const std::vector<t_lc>& LCs = OPT->LCs(_prn.system());
  for (unsigned ii = 0; ii < LCs.size(); ii++) {
    t_lc LC = LCs[ii];
    if (LC._type == t_lc::GIM) {
      continue;
    }
    if (LC._frq1 != t_frequency::G5 && !isValid(LC)) {
      _valid = false;
      return;
    }
  }

  // Find GLONASS Channel Number
  // ---------------------------
  if (_prn.system() == 'R') {
    _channel = PPP_CLIENT->ephPool()->getChannel(_prn);
  }
  else {
    _channel = 0;
  }

  // Compute Satellite Coordinates at Time of Transmission
  // -----------------------------------------------------
  _xcSat.ReSize(6); _xcSat = 0.0;
  _vvSat.ReSize(3); _vvSat = 0.0;
  bool totOK  = false;
  ColumnVector satPosOld(6); satPosOld = 0.0;

  _valid = false;
  t_frequency::type frq1 = t_frequency::dummy;
  t_frequency::type frq2 = t_frequency::dummy;
  OPT->defaultFrqs(_prn.system(), frq1, frq2);
  if (frq1 != t_frequency::dummy) {
    t_lc lc1(t_lc::code, frq1);
    if (isValid(lc1)) {
      _valid = true;
      _rangeLC = lc1;
    }
    if (frq2 != t_frequency::dummy) {
      t_lc lcIF(t_lc::codeIF, frq1, frq2);
      if (isValid(lcIF)) {
        _valid = true;
        _rangeLC = lcIF;
      }
    }
  }
  if (!_valid) {
    return;
  }
  
  double prange = obsValue(_rangeLC);
  for (int ii = 1; ii <= 10; ii++) {
    bncTime ToT = _time - prange / t_CST::c - _xcSat[3];
    if (PPP_CLIENT->ephPool()->getCrd(_prn, ToT, _xcSat, _vvSat) != success) {
      _valid = false;
      return;
    }
    ColumnVector dx = _xcSat - satPosOld;
    dx[3] *= t_CST::c;
    if (dx.NormFrobenius() < 1.e-4) {
      totOK = true;
      break;
    }
    satPosOld = _xcSat;
  }
  if (totOK) {
    _signalPropagationTime = prange / t_CST::c - _xcSat[3];
    _model._satClkM = _xcSat[3] * t_CST::c;
  }
  else {
    _valid = false;
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::lcCoeff(t_lc LC,
                          map<t_frequency::type, double>& codeCoeff,
                          map<t_frequency::type, double>& phaseCoeff,
                          map<t_frequency::type, double>& ionoCoeff) const {

  codeCoeff.clear();
  phaseCoeff.clear();
  ionoCoeff.clear();

  double f1 = t_CST::freq(LC._frq1, _channel);
  double f2 = t_CST::freq(LC._frq2, _channel);
  double f1GPS = t_CST::freq(t_frequency::G1, 0);

  switch (LC._type) {
  case t_lc::phase:
    phaseCoeff[LC._frq1] =  1.0;
    ionoCoeff [LC._frq1] = -1.0 * pow(f1GPS, 2) / pow(f1, 2);
    return;
  case t_lc::code:
    codeCoeff[LC._frq1] = 1.0;
    ionoCoeff[LC._frq1] = pow(f1GPS, 2) / pow(f1, 2);
    return;
  case t_lc::phaseIF:
    phaseCoeff[LC._frq1] =  f1 * f1 / (f1 * f1 - f2 * f2);
    phaseCoeff[LC._frq2] = -f2 * f2 / (f1 * f1 - f2 * f2);
    return;
  case t_lc::codeIF:
    codeCoeff[LC._frq1] =  f1 * f1 / (f1 * f1 - f2 * f2);
    codeCoeff[LC._frq2] = -f2 * f2 / (f1 * f1 - f2 * f2);
    return;
  case t_lc::MW:
    phaseCoeff[LC._frq1] =  f1 / (f1 - f2);
    phaseCoeff[LC._frq2] = -f2 / (f1 - f2);
    codeCoeff [LC._frq1] = -f1 / (f1 + f2);
    codeCoeff [LC._frq2] = -f2 / (f1 + f2);
    return;
  case t_lc::CL:
    phaseCoeff[LC._frq1] =  0.5;
    codeCoeff [LC._frq1] =  0.5;
    return;
  case t_lc::GIM:
  case t_lc::dummy:
  case t_lc::maxLc:
    return;
  }
}

//
////////////////////////////////////////////////////////////////////////////
bool t_pppSatObs::isValid(t_lc LC) const {
  bool valid = true;
  obsValue(LC, &valid);

  return valid;
}
//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::obsValue(t_lc LC, bool* valid) const {

  double retVal = 0.0;
  if (valid) *valid = true;

  // Pseudo observations
  if (LC._type == t_lc::GIM) {
    if (_stecSat == 0.0) {
      if (valid) *valid = false;
      return 0.0;
    }
    else {
      return _stecSat;
    }
  }

  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  lcCoeff(LC, codeCoeff, phaseCoeff, ionoCoeff);

  map<t_frequency::type, double>::const_iterator it;

  // Code observations
  for (it = codeCoeff.begin(); it != codeCoeff.end(); it++) {
    t_frequency::type tFreq = it->first;
    if (_obs[tFreq] == 0) {
      if (valid) *valid = false;
      return 0.0;
    }
    else {
      retVal += it->second * _obs[tFreq]->_code;
    }
  }
  // Phase observations
  for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
    t_frequency::type tFreq = it->first;
    if (_obs[tFreq] == 0) {
      if (valid) *valid = false;
      return 0.0;
    }
    else {
      retVal += it->second * _obs[tFreq]->_phase * t_CST::lambda(tFreq, _channel);
    }
  }
  return retVal;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::lambda(t_lc LC) const {

  double f1 = t_CST::freq(LC._frq1, _channel);
  double f2 = t_CST::freq(LC._frq2, _channel);

  if      (LC._type == t_lc::phase) {
    return t_CST::c / f1;
  }
  else if (LC._type == t_lc::phaseIF) {
    return t_CST::c / (f1 + f2);
  }
  else if (LC._type == t_lc::MW) {
    return t_CST::c / (f1 - f2);
  }
  else if (LC._type == t_lc::CL) {
    return t_CST::c / f1 / 2.0;
  }

  return 0.0;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::sigma(t_lc LC) const {

  double retVal = 0.0;
  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  lcCoeff(LC, codeCoeff, phaseCoeff, ionoCoeff);

  if (LC._type == t_lc::GIM) {
    retVal = OPT->_sigmaGIM * OPT->_sigmaGIM;
  }

  map<t_frequency::type, double>::const_iterator it;
  for (it = codeCoeff.begin(); it != codeCoeff.end(); it++)   {
    retVal += it->second * it->second * OPT->_sigmaC1 * OPT->_sigmaC1;
  }

  for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
    retVal += it->second * it->second * OPT->_sigmaL1 * OPT->_sigmaL1;
  }

  retVal = sqrt(retVal);

  // De-Weight R
  // -----------
  if (_prn.system() == 'R'&& LC.includesCode()) {
    retVal *= 5.0;
  }

  // Elevation-Dependent Weighting
  // -----------------------------
  double cEle = 1.0;
  if ( (OPT->_eleWgtCode  && LC.includesCode()) ||
       (OPT->_eleWgtPhase && LC.includesPhase()) ) {
    double eleD = eleSat()*180.0/M_PI;
    double hlp  = fabs(90.0 - eleD);
    cEle = (1.0 + hlp*hlp*hlp*0.000004);
  }

  return cEle * retVal;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::maxRes(t_lc LC) const {
  double retVal = 0.0;

  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  lcCoeff(LC, codeCoeff, phaseCoeff, ionoCoeff);

  map<t_frequency::type, double>::const_iterator it;
  for (it = codeCoeff.begin(); it != codeCoeff.end(); it++)   {
    retVal += it->second * it->second * OPT->_maxResC1 * OPT->_maxResC1;
  }
  for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
    retVal += it->second * it->second * OPT->_maxResL1 * OPT->_maxResL1;
  }
  if (LC._type == t_lc::GIM) {
    retVal = OPT->_maxResGIM * OPT->_maxResGIM + OPT->_maxResGIM * OPT->_maxResGIM;
  }

  retVal = sqrt(retVal);

  return retVal;
}


//
////////////////////////////////////////////////////////////////////////////
t_irc t_pppSatObs::cmpModel(const t_pppStation* station) {

  // Reset all model values
  // ----------------------
  _model.reset();

  // Topocentric Satellite Position
  // ------------------------------
  ColumnVector rSat = _xcSat.Rows(1,3);
  ColumnVector rRec = station->xyzApr();
  ColumnVector rhoV = rSat - rRec;
  _model._rho = rhoV.NormFrobenius();

  ColumnVector vSat = _vvSat;

  ColumnVector neu(3);
  xyz2neu(station->ellApr().data(), rhoV.data(), neu.data());

  _model._eleSat = acos(sqrt(neu[0]*neu[0] + neu[1]*neu[1]) / _model._rho);
  if (neu[2] < 0.0) {
    _model._eleSat *= -1.0;
  }
  _model._azSat  = atan2(neu[1], neu[0]);

  // Sun unit vector
  ColumnVector xSun = t_astro::Sun(_time.mjddec());
  xSun /= xSun.norm_Frobenius();

  // Satellite unit vectors sz, sy, sx
  ColumnVector sz = -rSat / rSat.norm_Frobenius();
  ColumnVector sy = crossproduct(sz, xSun);
  ColumnVector sx = crossproduct(sy, sz);

  sx /= sx.norm_Frobenius();
  sy /= sy.norm_Frobenius();

  // LOS unit vector satellite --> receiver
  ColumnVector rho = rRec - rSat;
  rho /= rho.norm_Frobenius();

  // LOS vector in satellite frame
  ColumnVector u(3);
  u(1) = dotproduct(sx, rho);
  u(2) = dotproduct(sy, rho);
  u(3) = dotproduct(sz, rho);

  // Azimuth and elevation in satellite antenna frame
  _model._elTx = atan2(u(3),sqrt(pow(u(2),2)+pow(u(1),2)));
  _model._azTx = atan2(u(2),u(1));


  // Satellite Clocks
  // ----------------
  _model._satClkM = _xcSat[3] * t_CST::c; // satellite system specific

  // Receiver Clocks
  // ---------------
  _model._recClkM = station->dClk() * t_CST::c;

  // Sagnac Effect (correction due to Earth rotation)
  // ------------------------------------------------
  ColumnVector Omega(3);
  Omega[0] = 0.0;
  Omega[1] = 0.0;
  Omega[2] = t_CST::omega / t_CST::c;
  _model._sagnac = DotProduct(Omega, crossproduct(rSat, rRec));

  // Antenna Eccentricity
  // --------------------
  _model._antEcc = -DotProduct(station->xyzEcc(), rhoV) / _model._rho;

  // Antenna Phase Center Offsets and Variations
  // -------------------------------------------
  if (PPP_CLIENT->antex()) {
    for (unsigned ii = 0; ii < t_frequency::max; ii++) {
      t_frequency::type frqType = static_cast<t_frequency::type>(ii);
      string frqStr = t_frequency::toString(frqType);
      if (frqStr[0] != _prn.system()) {continue;}
      bool found;
      QString prn(_prn.toString().c_str());
      _model._antPCO[ii]  = PPP_CLIENT->antex()->rcvCorr(station->antName(), frqType, _model._eleSat, _model._azSat, found);
      _model._antPCO[ii] += PPP_CLIENT->antex()->satCorr(prn, frqType, _model._elTx, _model._azTx, found);
      if (OPT->_isAPC && found) {
        // the PCOs as given in the satellite antenna correction for all frequencies
        // have to be reduced by the PCO of the respective reference frequency
        if      (_prn.system() == 'G') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::G1, _model._elTx, _model._azTx, found);
        }
        else if (_prn.system() == 'R') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::R1, _model._elTx, _model._azTx, found);
        }
        else if (_prn.system() == 'E') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::E1, _model._elTx, _model._azTx, found);
        }
        else if (_prn.system() == 'C') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::C2, _model._elTx, _model._azTx, found);
        }
      }
    }
  }

  // Tropospheric Delay
  // ------------------
  _model._tropo  = t_tropo::delay_saast(rRec, _model._eleSat);

  // Code Biases
  // -----------
  const t_satCodeBias* satCodeBias = PPP_CLIENT->obsPool()->satCodeBias(_prn);
  if (satCodeBias) {
    for (unsigned ii = 0; ii < satCodeBias->_bias.size(); ii++) {
      const t_frqCodeBias& bias = satCodeBias->_bias[ii];
      for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
        string frqStr = t_frequency::toString(t_frequency::type(iFreq));
        if (frqStr[0] != _prn.system()) {
          continue;
        }
        const t_frqObs* obs = _obs[iFreq];
        if (obs && obs->_rnxType2ch == bias._rnxType2ch) {
          _model._codeBias[iFreq] = (bias._value != 0.0 ? bias._value : ZEROVALUE);
        }
      }
    }
  }

  // Phase Biases
  // -----------
  double yaw    = 0.0;
  bool   useYaw = false;
  if (OPT->arSystem(_prn.system())) {
    const t_satPhaseBias* satPhaseBias = PPP_CLIENT->obsPool()->satPhaseBias(_prn);
    if (satPhaseBias) {
      if (OPT->_ar._useYaw) {
        double dt = station->epochTime() - satPhaseBias->_time;
        if (satPhaseBias->_updateInt) {
          dt -= (0.5 * ssrUpdateInt[satPhaseBias->_updateInt]);
        }
        yaw = satPhaseBias->_yaw + satPhaseBias->_yawRate * dt;
        useYaw = true;
      }
      for (unsigned ii = 0; ii < satPhaseBias->_bias.size(); ii++) {
        const t_frqPhaseBias& bias = satPhaseBias->_bias[ii];
        if (bias._fixIndicator) {  // if AR, biases without fixIndicator not used
          for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
            string frqStr = t_frequency::toString(t_frequency::type(iFreq));
            if (frqStr[0] != _prn.system()) {
              continue;
            }
            t_frqObs* obs = _obs[iFreq];
            if (obs && obs->_rnxType2ch[0] == bias._rnxType2ch[0]) { // allow different tracking mode
              _model._phaseBias[iFreq] = (bias._value != 0.0 ? bias._value : ZEROVALUE);
              obs->_biasJumpCounter = bias._jumpCounter;
            }
          }
        }
      }
    }
  }

  // Phase Wind-Up
  // -------------
  _model._windUp = station->windUp(_time, _prn, rSat, useYaw, yaw, vSat) ;

  // Relativistic effect due to earth gravity
  // ----------------------------------------
  double a = rSat.NormFrobenius() + rRec.NormFrobenius();
  double b = (rSat - rRec).NormFrobenius();
  double gm = 3.986004418e14; // m3/s2
  _model._rel = 2 * gm / t_CST::c / t_CST::c * log((a + b) / (a - b));

  // Tidal Correction
  // ----------------
  _model._tideEarth = -DotProduct(station->tideDsplEarth(), rhoV) / _model._rho;
  _model._tideOcean = -DotProduct(station->tideDsplOcean(), rhoV) / _model._rho;

  // Ionospheric Delay
  // -----------------
  const t_vTec* vTec = PPP_CLIENT->obsPool()->vTec();
  bool vTecUsage = true;
  for (unsigned ii = 0; ii < OPT->LCs(_prn.system()).size(); ii++) {
    t_lc LC = OPT->LCs(_prn.system())[ii];
    if (LC._type == t_lc::codeIF || LC._type == t_lc::phaseIF) {
      vTecUsage = false;
    }
  }

  if (vTecUsage && vTec) {
    double stec  = station->stec(vTec, _signalPropagationTime, rSat);
    double f1GPS = t_CST::freq(t_frequency::G1, 0);
    for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
      if (OPT->_pseudoObsIono) {
        // For scaling the slant ionospheric delays the trick is to be consistent with units!
        // The conversion of TECU into meters requires the frequency of the signal.
        // Hence, GPS L1 frequency is used for all systems. The same is true for mu_i in lcCoeff().
        _model._ionoCodeDelay[iFreq] = 40.3E16 / pow(f1GPS, 2) * stec;
      }
      else { // PPP-RTK
        t_frequency::type frqType = static_cast<t_frequency::type>(iFreq);
        _model._ionoCodeDelay[iFreq] = 40.3E16 / pow(t_CST::freq(frqType, _channel), 2) * stec;
      }
    }
  }

  // Set Model Set Flag
  // ------------------
  _model._set = true;

  if (OPT->_logMode == t_pppOptions::all) {
    printModel();
  }

  return success;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::printModel() const {

  LOG.setf(ios::fixed);
  LOG << "\nMODEL for Satellite " << _prn.toString() << (isReference() ? " (Reference Satellite)" : "")

      << "\n======================= " << endl
      << "PPP "
      <<  ((OPT->_pseudoObsIono) ? " with pseudo-observations for STEC" : "")          << endl
      << "RHO           : " << setw(12) << setprecision(3) << _model._rho              << endl
      << "ELE           : " << setw(12) << setprecision(3) << _model._eleSat * RHO_DEG << endl
      << "AZI           : " << setw(12) << setprecision(3) << _model._azSat  * RHO_DEG << endl
      << "SATCLK        : " << setw(12) << setprecision(3) << _model._satClkM          << endl
      << "RECCLK        : " << setw(12) << setprecision(3) << _model._recClkM          << endl
      << "SAGNAC        : " << setw(12) << setprecision(3) << _model._sagnac           << endl
      << "ANTECC        : " << setw(12) << setprecision(3) << _model._antEcc           << endl
      << "TROPO         : " << setw(12) << setprecision(3) << _model._tropo            << endl
      << "WINDUP        : " << setw(12) << setprecision(3) << _model._windUp           << endl
      << "REL           : " << setw(12) << setprecision(3) << _model._rel              << endl
      << "EARTH TIDES   : " << setw(12) << setprecision(3) << _model._tideEarth        << endl
      << "OCEAN TIDES   : " << setw(12) << setprecision(3) << _model._tideOcean        << endl
      << endl
      << "FREQUENCY DEPENDENT CORRECTIONS:" << endl
      << "-------------------------------" << endl;
  for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
    if (_obs[iFreq]) {
      string frqStr = t_frequency::toString(t_frequency::type(iFreq));
      if (_prn.system() == frqStr[0]) {
      LOG << "PCO           : " << frqStr << setw(12) << setprecision(3) << _model._antPCO[iFreq]       << endl
          << "BIAS CODE     : " << frqStr << setw(12) << setprecision(3) << _model._codeBias[iFreq]     << "\t(" << _obs[iFreq]->trkChar() << ") " << endl
          << "BIAS PHASE    : " << frqStr << setw(12) << setprecision(3) << _model._phaseBias[iFreq]    << "\t(" << _obs[iFreq]->trkChar() << ") " << endl
          << "IONO CODEDELAY: " << frqStr << setw(12) << setprecision(3) << _model._ionoCodeDelay[iFreq]<< endl;
      }
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::printObsMinusComputed() const {
  LOG.setf(ios::fixed);
  LOG << "\nOBS-COMP for Satellite " << _prn.toString() << (isReference() ? " (Reference Satellite)" : "") << endl
       << "========================== " << endl;
  char sys = _prn.system();
  for (unsigned ii = 0; ii < OPT->LCs(sys).size(); ii++) {
    t_lc LC = OPT->LCs(sys)[ii];
    LOG << "OBS-CMP " << setw(4) << LC.toString() << ": " << _prn.toString() << " "
        << setw(12) << setprecision(3) << obsValue(LC) << " "
        << setw(12) << setprecision(3) << cmpValue(LC) << " "
        << setw(12) << setprecision(3) << obsValue(LC)  - cmpValue(LC) << endl;
  }
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::cmpValueForBanc(t_lc LC) const {
  return cmpValue(LC) - _model._rho - _model._sagnac - _model._recClkM;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::cmpValue(t_lc LC) const {
  double cmpValue;

  if      (!isValid(LC)) {
    cmpValue =  0.0;
  }
  else if (LC._type == t_lc::GIM) {
    cmpValue =  _stecSat;
  }
  else {
    // Non-Dispersive Part
    // -------------------
    double nonDisp = _model._rho
                   + _model._recClkM   - _model._satClkM
                   + _model._sagnac    + _model._antEcc    + _model._tropo
                   + _model._tideEarth + _model._tideOcean + _model._rel;

    // Add Dispersive Part
    // -------------------
    double dispPart = 0.0;
    map<t_frequency::type, double> codeCoeff;
    map<t_frequency::type, double> phaseCoeff;
    map<t_frequency::type, double> ionoCoeff;
    lcCoeff(LC, codeCoeff, phaseCoeff, ionoCoeff);
    map<t_frequency::type, double>::const_iterator it;
    for (it = codeCoeff.begin(); it != codeCoeff.end(); it++) {
      t_frequency::type tFreq = it->first;
      dispPart += it->second * (_model._antPCO[tFreq] - _model._codeBias[tFreq]);
    }
    for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
      t_frequency::type tFreq = it->first;
      dispPart += it->second * (_model._antPCO[tFreq] - _model._phaseBias[tFreq] +
                                _model._windUp * t_CST::lambda(tFreq, _channel));
    }
    cmpValue = nonDisp + dispPart;
  }

  return cmpValue;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::setRes(t_lc LC, double res) {
  _res[LC] = res;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::getRes(t_lc LC) const {
  map<t_lc, double>::const_iterator it = _res.find(LC);
  if (it != _res.end()) {
    return it->second;
  }
  else {
    return 0.0;
  }
}

//
////////////////////////////////////////////////////////////////////////////
bool  t_pppSatObs::setPseudoObsIono(t_frequency::type freq) {
  bool pseudoObsIono = false;
  _stecSat    = _model._ionoCodeDelay[freq];
  if (_stecSat) {
    pseudoObsIono = true;
  }
  return pseudoObsIono;
}

//
////////////////////////////////////////////////////////////////////////////
bool t_pppSatObs::hasBiases() const {
  bool ar = OPT->arSystem(_prn.system());
  set<t_frequency::type> frqs;
  for (const auto& lc : OPT->LCs(_prn.system())) {
    if (lc._frq1 != t_frequency::dummy) frqs.insert(lc._frq1);
    if (lc._frq2 != t_frequency::dummy) frqs.insert(lc._frq2);
  }
  for (int iFreq : frqs) {
    if (_obs[iFreq] != 0) {
      if (_model._codeBias[iFreq] == 0 || (ar && _model._phaseBias[iFreq] == 0)) {
        return false;
      }
    }
  }
  return true;
}
