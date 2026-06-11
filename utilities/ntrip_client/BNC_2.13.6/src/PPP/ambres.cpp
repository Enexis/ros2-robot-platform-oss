#include <set>
#include <algorithm>
#include <math.h>
#include <newmatio.h>

#include "ambres.h"
#include "bncutils.h"
#include "lambda.h"
#include "pppClient.h"

using namespace std;
using namespace BNC_PPP;

const double AmbRes::sigCon = 1e-4;

//
//////////////////////////////////////////////////////////////////////////////////////////
AmbRes::AmbRes() {
  reset();
}

//
//////////////////////////////////////////////////////////////////////////////////////////
AmbRes::~AmbRes() {
}

//
//////////////////////////////////////////////////////////////////////////////////////////
bool AmbRes::isResolvable(const t_pppParam* amb) {
  if ( amb->ambNumEpo() >= OPT->_ar._minNumEpo && amb->ambEleSat() >= OPT->_ar._minEle) {
    return true;
  }
  else {
    return false;
  }
}

//
//////////////////////////////////////////////////////////////////////////////////////////
bool AmbRes::isFixable(double xx, double sigma, bool* checked) {
  if (OPT->_ar._maxFrac <= 0.0 || OPT->_ar._maxSig <= 0.0) {
    if (checked) {
      *checked = false;
    }
    return true;
  }
  else {
    if (checked) {
      *checked = true;
    }
    return  ( fabs(xx - nint(xx)) <= OPT->_ar._maxFrac && sigma <= OPT->_ar._maxSig );
  }
}

//
//////////////////////////////////////////////////////////////////////////////////////////
void AmbRes::addToGroup(const t_pppParam* amb) {
  t_lc LC  = amb->LC();
  char       sys = amb->prn().system();
  AmbGroup* p_ambGroup = 0;
  for (AmbGroup& ambGroup : _ambGroups) {
    if (ambGroup._system == sys && ambGroup._LC == LC) {
      p_ambGroup = &ambGroup;
      break;
    }
  }
  if (!p_ambGroup) {
    _ambGroups.emplace_back(AmbGroup(sys, LC));
    p_ambGroup = &_ambGroups.back();
  }
  p_ambGroup->_zdAmbs.push_back(ZdAmb(amb));

  _numZdAmbs += 1;
}

// Select Reference Ambiguity
////////////////////////////////////////////////////////////////////////////////////////////
void AmbRes::selectReference(AmbGroup& ambGroup) {

  // Compute sum of variances of all double-difference ambiguities
  // -------------------------------------------------------------
  double minVarSum = 0.0;
  for (const ZdAmb& zdAmb1 : ambGroup._zdAmbs) {
    int i1 = zdAmb1.index();
    double varSum = 0.0;
    for (const ZdAmb& zdAmb2 : ambGroup._zdAmbs) {
      int i2 = zdAmb2.index();
      varSum += _QQ[i1][i1] - 2.0 * _QQ(i1+1,i2+1) + _QQ[i2][i2];
    }
    if (ambGroup._zdAmbRef == 0 || minVarSum > varSum) {
      ambGroup._zdAmbRef  = &zdAmb1;
      minVarSum           = varSum;
    }
  }
}

//
//////////////////////////////////////////////////////////////////////////////////////////
t_irc AmbRes::run(const bncTime&                  epoTime, 
                  const std::vector<t_pppParam*>& params,
                  SymmetricMatrix&                QFinal, 
                  ColumnVector&                   xFinal,
                  double&                         fixRatio,
                  std::ostream&                   msg) {
  reset();
  fixRatio = 0.0;
  
  msg << string(epoTime) << " Ambiguity Resolution (BIE)" << endl;
  
  // Resolvable ambiguities
  // ----------------------
  map<char, set<t_prn>> usedPrns;
  for (char system : OPT->_ar._systems) {
    for (const auto& par : params) {
      if (par->type() == t_pppParam::amb && par->prn().system() == system) {
        if (isResolvable(par)) {
          addToGroup(par);
        }
      }
    }
  }

  // Remove Groups with less than 2 ambiguities
  // ------------------------------------------
  auto iGrp = _ambGroups.begin();
  while (iGrp != _ambGroups.end()) {
    if (iGrp->_zdAmbs.size() < 2) {
      _numZdAmbs -= iGrp->_zdAmbs.size();
      iGrp = _ambGroups.erase(iGrp);
    }
    else {
      ++iGrp;
    }
  }

  // Check number of zero-difference ambiguities
  // -------------------------------------------
  bool hasEnoughAmbs = false;
  for (const auto& ambGroup : _ambGroups) {
    if (int(ambGroup._zdAmbs.size()) >= OPT->_ar._minNumSat) {
      hasEnoughAmbs = true;
      break;
    }
  }
  if (!hasEnoughAmbs) {
    msg << "    not enough resolvable ambiguities" << endl;
    return t_irc::failure;
  }

  // Construct the variance-covariance matrix of ambiguities
  // -------------------------------------------------------
  Matrix ZD(_numZdAmbs, params.size()); ZD = 0.0;
  int idx = -1;
  for (auto& ambGroup : _ambGroups) {
    for (auto& zdAmb : ambGroup._zdAmbs) {
      idx += 1;
      zdAmb.setIndex(idx);
      ZD[idx][zdAmb.indexGlobal()] = 1.0;
    }
  }
  _xx =  ZD * xFinal;
  _QQ << ZD * QFinal * ZD.t();

  // Select and Constrain Reference Ambiguities
  // ------------------------------------------
  Matrix       HH(_ambGroups.size(), _numZdAmbs); HH = 0.0;
  ColumnVector hh(_ambGroups.size());
  for (unsigned iGrp = 0; iGrp < _ambGroups.size(); ++iGrp) {
    AmbGroup& ambGroup = _ambGroups[iGrp];
    selectReference(ambGroup);
    HH[iGrp][ambGroup._zdAmbRef->index()] = 1.0;
    hh[iGrp]                              = nint(_xx[ambGroup._zdAmbRef->index()]);
  }
  DiagonalMatrix PP(HH.nrows()); PP = 1.0 / (sigCon * sigCon);
  kalman(HH, hh, PP, _QQ, _xx);
  
  // Perform the Lambda (BIE) Search
  // -------------------------------
  ColumnVector    xBie;
  SymmetricMatrix covBie;
  Lambda::search(_xx, _QQ, xBie, covBie);
  if (xBie.Nrows() == 0) {
    msg << " BIE search: failed" << endl; 
    return t_irc::failure;
  }

  // Print BIE Results
  // -----------------
  printBieResults(xBie, covBie, msg);

  // Constrain ambiguities
  // ---------------------
  setConstraints(QFinal, xFinal, xBie, covBie, fixRatio, msg);

  return t_irc::success;
}

// Print single-difference ambiguities and AR results
////////////////////////////////////////////////////////////////////////////////////////////
void AmbRes::printBieResults(const ColumnVector& xBie, const SymmetricMatrix& covBie,
                             ostream& msg) const {
  msg.setf(ios::fixed);
  for (const AmbGroup& ambGroup : _ambGroups) {
    msg <<  "  Group " << ambGroup._LC.toString() << endl;
    
    for (const ZdAmb& zdAmb: ambGroup._zdAmbs) {
      int idx = zdAmb.index();
      msg << "  " << zdAmb.prn().toString() << ' ' << setw(7) << setprecision(2) << _xx[idx];
      if (&zdAmb == ambGroup._zdAmbRef) {
        msg << " ref";
      }
      else {
        msg << " +- ";
      }
      msg << setw(6) << setprecision(2) << sqrtMod(_QQ[idx][idx]) << ' '
          << setw(7) << setprecision(2) << xBie[idx] << ' '
          << setw(9) << setprecision(6) << sqrtMod(covBie[idx][idx]);
      if (isFixable(xBie[idx], sqrtMod(covBie[idx][idx]))) {
        msg << " fix ";
      }
      msg << endl;
    }
  }
}

// 
////////////////////////////////////////////////////////////////////////////////////////////
void AmbRes::setConstraints(SymmetricMatrix& QFinal, ColumnVector& xFinal,
                            const ColumnVector& xBie, const SymmetricMatrix& covBie, 
                            double& fixRatio, ostream& /*msg*/) {

  fixRatio = 0.0;
  
  vector<unique_ptr<const RowVector>> CC;
  vector<double>                      cc;
  vector<double>                      Sc;
  int numSdAmbs   = 0;
  int numFixSdAll = 0;
  bool enoughAmbs = false;
  for (const AmbGroup& ambGroup : _ambGroups) {
    numSdAmbs += ambGroup._zdAmbs.size() - 1;
    int numFixSdGrp = 0;
    for (const ZdAmb& zdAmb : ambGroup._zdAmbs) {
      int idx = zdAmb.index();
      if (isFixable(xBie[idx], sqrtMod(covBie[idx][idx]))) {
        if (ambGroup._zdAmbRef->indexGlobal() != zdAmb.indexGlobal()) {
          numFixSdAll += 1;
          numFixSdGrp += 1;
        }
        RowVector* pCC;
        CC.emplace_back(pCC = new RowVector(xFinal.Nrows()));
        cc.push_back(xBie[idx]);
        Sc.push_back(sigCon);
        (*pCC)                      = 0.0;
        (*pCC)[zdAmb.indexGlobal()] = 1.0;
      }
    }
    if (numFixSdGrp >= OPT->_ar._minNumSat-1) {
      enoughAmbs = true;
    }
  }
  if (enoughAmbs) {
    kalman(CC, cc, Sc, QFinal, xFinal);
    fixRatio = double(numFixSdAll) / double(numSdAmbs);
  }
}
