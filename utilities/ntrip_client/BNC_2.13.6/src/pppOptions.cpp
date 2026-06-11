// Part of BNC, a utility for retrieving decoding and
// converting GNSS data streams from NTRIP broadcasters.
//
// Copyright (C) 2007
// German Federal Agency for Cartography and Geodesy (BKG)
// http://www.bkg.bund.de
// Czech Technical University Prague, Department of Geodesy
// http://www.fsv.cvut.cz
//
// Email: euref-ip@bkg.bund.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppOptions
 *
 * Purpose:    Options for PPP client
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <QtCore>
#include <set>
#include <newmatio.h>
#include "pppOptions.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_pppOptions::t_pppOptions() {
  _xyzAprRover.ReSize(3); _xyzAprRover = 0.0;
  _neuEccRover.ReSize(3); _neuEccRover = 0.0;
  _aprSigCrd.ReSize(3);   _aprSigCrd   = 0.0;
  _noiseCrd.ReSize(3);    _noiseCrd    = 0.0;
}

// Destructor
//////////////////////////////////////////////////////////////////////////////
t_pppOptions::~t_pppOptions() {
}

//
//////////////////////////////////////////////////////////////////////////////
const std::vector<t_lc>& t_pppOptions::LCs(char system) const {

  if      (system == 'R') {
    return _LCsGLONASS;
  }
  else if (system == 'E') {
    return _LCsGalileo;
  }
  else if (system == 'C') {
    return  _LCsBDS;
  }
  else {
    return _LCsGPS;
  }
}

//
//////////////////////////////////////////////////////////////////////////////
bool t_pppOptions::useOrbClkCorr() const {
  if (_realTime) {
    return !_corrMount.empty();
  }
  else {
    return !_corrFile.empty();
  }
}

// Processed satellite systems
/////////////////////////////////////////////////////////////////////////////
vector<char> t_pppOptions::systems() const {
  vector<char> answ;
  if (_LCsGPS.size()     > 0) answ.push_back('G');
  if (_LCsGLONASS.size() > 0) answ.push_back('R');
  if (_LCsGalileo.size() > 0) answ.push_back('E');
  if (_LCsBDS.size()     > 0) answ.push_back('C');
  return answ;
}

//
/////////////////////////////////////////////////////////////////////////////
vector<t_lc> t_pppOptions::ambLCs(char system) const {

  set<t_frequency::type> frqs;

  int numPhaseLCs = 0;
  const vector<t_lc>& allLCs = LCs(system);
  for (unsigned ii = 0; ii < allLCs.size(); ii++) {
    const t_lc& LC = allLCs[ii];
    if (LC.includesPhase()) {
      numPhaseLCs += 1;
      frqs.insert(LC._frq1);
      if (LC._frq2 != t_frequency::dummy) {
        frqs.insert(LC._frq2);
      }
    }
  }

  vector<t_lc> answ;
  for (auto it = frqs.begin(); it != frqs.end(); ++it) {
    answ.push_back(t_lc(t_lc::phase, *it));
    if (numPhaseLCs == 1) {
      break;
    }
  }

  return answ;
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppOptions::setTrkModes(const std::string& trkStr) {

  QStringList priorList = QString::fromStdString(trkStr).split(" ", Qt::SkipEmptyParts);
  for (int ii = 0; ii < priorList.size(); ii++) {
    if (priorList[ii].indexOf(":") != -1) {
      QStringList sysList = priorList[ii].split(":", Qt::SkipEmptyParts);
      if (sysList.size() == 2 && sysList[0].length() == 1) {
        char sys = sysList[0].toStdString()[0];
        SysTrkModes& sysTrkModes = _trkModesMap[sys];
        QStringList hlpList = sysList[1].split("&", Qt::SkipEmptyParts);
        if (hlpList.size() == 2) {
          string frqStr = hlpList[0].toStdString();
          string trkStr = hlpList[1].toStdString();
          for (unsigned jj = 0; jj < frqStr.length(); jj++) {
            char frqChar = frqStr[jj];
            t_frequency::type frq = t_frequency::toFreq(sys, frqChar);
            sysTrkModes._frqTrkModes.push_back(SysTrkModes::FrqTrkModes(frq, trkStr));
          }
        }
      }
    }
  }

  //// beg test
  // for (const auto& [key, value] : _trkModesMap) {
  //   cout << "system: " << key << endl;
  //   for (const auto& frqTrk : value._frqTrkModes) {
  //     cout << "    freq: "  << t_frequency::toString(frqTrk._frq) << ' '
  //          << frqTrk._trkModes << endl;
  //   }
  // }
  //// end test
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppOptions::defaultFrqs(char sys, t_frequency::type& frq1, t_frequency::type& frq2) const {

  frq1 = t_frequency::dummy;
  frq2 = t_frequency::dummy;

  const SysTrkModes* sysTrkModes = this->sysTrkModes(sys);

  if (sysTrkModes) {
    if (sysTrkModes->_frqTrkModes.size() > 0) {
      frq1 = sysTrkModes->_frqTrkModes[0]._frq;
    }
    if (sysTrkModes->_frqTrkModes.size() > 1) {
      frq2 = sysTrkModes->_frqTrkModes[1]._frq;
    }
  }
  else {
    if      (sys == 'G') {
      frq1 = t_frequency::G1;
      frq2 = t_frequency::G2;
    }
    else if (sys == 'R') {
      frq1 = t_frequency::R1;
      frq2 = t_frequency::R2;
    }
    else if (sys == 'E') {
      frq1 = t_frequency::E1;
      frq2 = t_frequency::E5;
    }
    else if (sys == 'C') {
      frq1 = t_frequency::C1;
      frq2 = t_frequency::C5;
    }
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppOptions::setLCs(char sys, const std::string& lcStr) {

  std::vector<t_lc>* LCs = 0;
  if      (sys == 'G') {
    LCs = &_LCsGPS;
  }
  else if (sys == 'R') {
    LCs = &_LCsGLONASS;
  }
  else if (sys == 'E') {
    LCs = &_LCsGalileo;
  }
  else if (sys == 'C') {
    LCs = &_LCsBDS;
  }
  else {
    return;
  }

  t_frequency::type frq1 = t_frequency::dummy;
  t_frequency::type frq2 = t_frequency::dummy;
  defaultFrqs(sys, frq1, frq2);

  if      (lcStr == "Pi&Li") {
    if (frq1 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::code,  frq1));
    if (frq2 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::code,  frq2));
    if (frq1 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::phase, frq1));
    if (frq2 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::phase, frq2));
    if (_pseudoObsIono) {
      LCs->push_back(t_lc(t_lc::GIM, t_frequency::dummy));
    }
  }
  else if (lcStr == "Pi") {
    if (frq1 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::code, frq1));
    if (frq2 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::code, frq2));
    if (_pseudoObsIono) {
      LCs->push_back(t_lc(t_lc::GIM, t_frequency::dummy));
    }
  }
  else if (lcStr == "P1&L1") {
    if (frq1 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::code,  frq1));
    if (frq1 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::phase, frq1));
    if (_pseudoObsIono) {
      LCs->push_back(t_lc(t_lc::GIM, t_frequency::dummy));
    }
  }
  else if (lcStr == "P1") {
    if (frq1 != t_frequency::dummy) LCs->push_back(t_lc(t_lc::code, frq1));
    if (_pseudoObsIono) {
      LCs->push_back(t_lc(t_lc::GIM, t_frequency::dummy));
    }
  }
  else if (lcStr == "P3&L3") {
    if (frq1 != t_frequency::dummy && frq2 != t_frequency::dummy) {
      LCs->push_back(t_lc(t_lc::codeIF,  frq1, frq2));
      LCs->push_back(t_lc(t_lc::phaseIF, frq1, frq2));
    }
  }
  else if (lcStr == "P3") {
    if (frq1 != t_frequency::dummy && frq2 != t_frequency::dummy) {
      LCs->push_back(t_lc(t_lc::codeIF, frq1, frq2));
    }
  }
  else if (lcStr == "L3") {
    if (frq1 != t_frequency::dummy && frq2 != t_frequency::dummy) {
      LCs->push_back(t_lc(t_lc::phaseIF, frq1, frq2));
    }
  }
  else {
    QStringListIterator it(QString(lcStr.c_str()).split("&", Qt::SkipEmptyParts));
    while (it.hasNext()) {
      string hlp = it.next().toStdString();
      for (unsigned ii = 1; ii < hlp.length(); ++ii) {
        t_frequency::type frq = t_frequency::toFreq(sys, hlp[ii]);
        if (frq != t_frequency::dummy) {
          if      (hlp[0] == 'P') {
            LCs->push_back(t_lc(t_lc::code, frq));
          }
          else if (hlp[0] == 'L') {
            LCs->push_back(t_lc(t_lc::phase, frq));
          }
        }
      }
    }
  }
}
