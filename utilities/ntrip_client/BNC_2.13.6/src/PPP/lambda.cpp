
#include <iostream>
#include <iomanip>
#include <newmatio.h>
#include <cmath>

#include "lambda.h"

using namespace BNC_PPP;
using namespace std;

// Gauss Error Function
/////////////////////////////////////////////////////////////////////////////////////////
double Lambda::erf(double xx) {
  static const double a1 =  0.254829592;
  static const double a2 = -0.284496736;
  static const double a3 =  1.421413741;
  static const double a4 = -1.453152027;
  static const double a5 =  1.061405429;
  static const double pp =  0.3275911;

  int sign = (xx < 0) ? -1 : 1;
  xx = fabs(xx);

  double tt = 1.0/(1.0 + pp*xx);
  double yy = 1.0 - (((((a5*tt + a4)*tt) + a3)*tt + a2)*tt + a1)*tt*exp(-xx*xx);

  return sign * yy;
}

// Cumulative Distribution Function (Normal Distribution)
/////////////////////////////////////////////////////////////////////////////////////////
double Lambda::normcdf(double x, double mu, double sigma) {
  static const double root_two = sqrt(2.0);
  return 0.5 * ( 1.0 + erf( (x-mu) / (sigma*root_two) ) );
}

// Auxiliary functions
/////////////////////////////////////////////////////////////////////////////////////////
void Lambda::swap(double& a, double& b) {
  double t(a); a = b; b = t; 
}
double Lambda::sign(double a) {
  if      (a < 0.0) {
    return -1.0;
  }
  else if (a > 0.0) {
    return 1.0;
  }
  else {
    return 0.0;
  }
}
double Lambda::nint(double val) {
  return ((val < 0.0) ? -floor(fabs(val)+0.5) : floor(val+0.5));
}

// LAMBDA/BIE Search
/////////////////////////////////////////////////////////////////////////////////////////
void Lambda::search(ColumnVector aFlt, const SymmetricMatrix& QQ,
                    ColumnVector& aFix, SymmetricMatrix& covBie) {

  int nn = QQ.Nrows();

  // Remove integer numbers from float solution (for computational convenience only)
  // -------------------------------------------------------------------------------
  ColumnVector incr(nn);
  for (int ii = 0; ii < nn; ii++) {
    incr[ii] = nint(aFlt[ii]);
    aFlt[ii] = aFlt[ii] - incr[ii];
  }
  
  // Compute ZZ matrix based on the decomposition  Q=LL^T*D*LL; The transformed
  // float solution: zFlt = ZZ^T *aFlt, QzFlt = ZZ^T * QQ * ZZ
  // -----------------------------------------------------------------------
  SymmetricMatrix       QzFlt;
  Matrix                ZZ;
  LowerTriangularMatrix LL;
  DiagonalMatrix        DD;
  ColumnVector          zFlt;
  Matrix                iZt;
  decorrel(QQ, aFlt, QzFlt, ZZ, LL, DD, zFlt, iZt);
  
  // Perform the search
  // ------------------
  Info info;
  BIE(zFlt, LL, DD, info);

  // Perform the back-transformation and add the increments
  // ------------------------------------------------------
  aFix = iZt * info.zBie + incr;
  info.zBie = ZZ.t() * aFix;
  info.aFix = iZt * info.zFix; 
  for (int iCand = 1; iCand <= info.zFix.Ncols(); iCand++) {
    info.aFix.column(iCand) += incr;
  }
  info.zFix = ZZ.t() * info.aFix;

#ifdef LAMBDA_MAIN_TEST
  cout.setf(ios::fixed);
  cout << "aFix(3 cand)= \n"  << setw(10) << setprecision(2) << info.aFix.columns(1,3) << endl;
#endif  
  
  // Variances of ambiguities
  // ------------------------
  static const double sigCon = 1e-6;
  DiagonalMatrix covZ(info.zFix.Nrows()); covZ = 0.0;
  for (int ia = 0; ia < info.zFix.Nrows(); ia++) { // loop over all ambiguities
    for (int ic = 0; ic < info.zFix.Ncols(); ic++) { // loop over all candidates
      double dZ = info.zBie[ia] - info.zFix[ia][ic];
      covZ[ia] += info.wgt[ic] * dZ * dZ;
    }
    if (covZ[ia] < sigCon*sigCon) { // make the matrix positive-definite
      covZ[ia] = sigCon*sigCon;
    }
  }
  Matrix invZ = ZZ.i();
  covBie << invZ.t() * covZ * invZ;
}

// Best Integer Equivariant Estimator
/////////////////////////////////////////////////////////////////////////////////////////
void Lambda::BIE(const ColumnVector& zFlt,        // Original ambiguities
                 const LowerTriangularMatrix& LL, // L matrix from L'DL-decomposition of QzFlt
                 const DiagonalMatrix& DD,        // D matrix from L'DL-decomposition of QzFlt
                 Info& info) {                  

  int          nn     = zFlt.Nrows();
  int          ncands = 100;
  ColumnVector sqnorm;
  info.wgt.ReSize(ncands); info.wgt  = 0.0;
  info.zBie.ReSize(nn);    info.zBie = 0.0;

  ssearch(zFlt, LL, DD, ncands, info.zFix, sqnorm);

  LowerTriangularMatrix Li = LL.i();
  SymmetricMatrix QzFltInv; QzFltInv << Li * DD.i() * Li.t();

  info.zBie.ReSize(nn); info.zBie = 0.0;
  double wgtSum = 0.0;
  double norm1  = 0.0;
  for (int ic = 1; ic <= ncands; ic++) {

    ColumnVector da     = zFlt - info.zFix.column(ic);
    double       daNorm = DotProduct(da, QzFltInv * da);

    if (ic == 1) {
      norm1 = daNorm;
      info.wgt(ic) = 1.0;
    }
    else {
      info.wgt(ic) = exp(-0.5 * (daNorm-norm1));  // weights scaled by exp(0.5 * norm1)
    }

    wgtSum += info.wgt(ic);

    for (int ia = 1; ia <= info.zFix.Nrows(); ia++) {
      info.zBie(ia) += info.wgt(ic) * info.zFix(ia,ic);
    }
  }

  info.zBie /= wgtSum;
  info.wgt  /= wgtSum;
}

// Decomposition Q = L'DL  (L is lower triangular)
/////////////////////////////////////////////////////////////////////////////////////////
void Lambda::ldldecom(const SymmetricMatrix& QQ, LowerTriangularMatrix& LL, DiagonalMatrix& DD) {

  const int n = QQ.Nrows();

  Matrix QC = QQ;

  LL.ReSize(n); LL = 0.0;
  DD.resize(n); DD = 0.0;
  
  for (int i = n-1; i >= 0; i--) {
    DD[i] = QC[i][i];
    if ( DD[i] <= 0.0 ) {
      throw "ldldecom problem";
    }
    double temp = sqrt(DD[i]);
    for (int j = 0; j <= i; j++) {
      LL[i][j] = QC[i][j]/temp;
    }
    for (int j = 0; j <= i-1; j++) {
      for(int k = 0; k <= j; k++) {
        QC[j][k] -= LL[i][k] * LL[i][j];
      }
    }
    for (int j = 0; j <= i; j++) {
      LL[i][j] /= LL[i][i];
    }
  }
}

// 
/////////////////////////////////////////////////////////////////////////////////////////
void Lambda::decorrel(const SymmetricMatrix& QQ, // Variance-covariance matrix of ambiguities
                        const ColumnVector& aFlt,  // Original ambiguities
                        SymmetricMatrix& QzFlt,    // Cov. matrix of decorrelated ambiguities
                        Matrix& ZZ,                // ZZ-transformation matrix
                        LowerTriangularMatrix& LL, // L matrix from L'DL-decomposition of QzFlt
                        DiagonalMatrix& DD,        // D matrix from L'DL-decomposition of QzFlt
                        ColumnVector& zFlt,        // Transformed ambiguities
                        Matrix& iZt) {             // ZZ.t().i() transformation matrix

  // L'DL Decomposition
  // ------------------
  ldldecom(QQ, LL, DD);

  // Reduction
  // ---------
  int n = DD.Nrows();

  iZt.ReSize(n,n); iZt = 0.0; 
  for (int i = 0; i < n; i++) {
    iZt[i][i] = 1.0;
  }

  int  i1 = n - 1;
  bool sw = true;
  while (sw) {

    int i = n;   // loop for column from n to 1
    sw = false;

    while ( !sw && i > 1) {

      i = i - 1;  // the ith column
      if (i <= i1) {
        for (int j = i+1; j <= n; j++) {
          double mu = nint(LL(j,i));
          if (mu != 0.0) {
            for (int k = j; k <= n; k++) {
              LL(k,i) = LL(k,i) - mu * LL(k,j);
            }
            for (int k = 1; k <= n; k++) {
              iZt(k,j) = iZt(k,j) + mu * iZt(k,i);
            }
          }
        }
      }

      double delta = DD(i) + LL(i+1,i) * LL(i+1,i) * DD(i+1);
      if (delta < DD(i+1)) {
        double lambda = DD(i+1) * LL(i+1,i) / delta;
        double eta    = DD(i) / delta;
        DD(i)         = eta * DD(i+1);
        DD(i+1)       = delta;

        Matrix hlp(2,2); hlp << -LL(i+1,i) << 1.0
                             <<     eta    << lambda;

        LL.submatrix(i,i+1,1,i-1) = hlp * LL.submatrix(i,i+1,1,i-1);

        LL(i+1,i) = lambda;

        for (int k = i+2; k <= n; k++) swap( LL(k,i),  LL(k,i+1));
        for (int k = 1;   k <= n; k++) swap(iZt(k,i), iZt(k,i+1));
        i1 = i;
        sw = true;
      }
    }
  }

  // Transformed Q-matrix, transformation-matrix, and decorrelated ambiguities
  // -------------------------------------------------------------------------
  ZZ = iZt.i().t();
  for (int i = 0; i < ZZ.Nrows(); i++) {
    for (int j = 0; j < ZZ.Nrows(); j++) {
      ZZ[i][j] = nint(ZZ[i][j]);
    }
  }

  QzFlt << ZZ.t() * QQ * ZZ;  // it is also L'DL
  zFlt = ZZ.t() * aFlt;
}

// Integer ambiguity vector search by employing the search-and-shrink technique
/////////////////////////////////////////////////////////////////////////////////////////
void Lambda::ssearch(const ColumnVector& zFlt,        // Original ambiguities
                       const LowerTriangularMatrix& LL, // L matrix from L'DL-decomposition of QzFlt
                       const DiagonalMatrix& DD,        // D matrix from L'DL-decomposition of QzFlt
                       int ncands,                      // Number of requested candidates
                       Matrix& zFix,                    // estimated integers (n x ncands )   
                       ColumnVector& sqnorm) {          // squared norms (ascendantly sorted)

  // Initialize outputs
  // ------------------
  int n = zFlt.Nrows();
  zFix.ReSize(n, ncands); zFix   = 0.0;
  sqnorm.ReSize(ncands);  sqnorm = 0.0;
  
  // Initializing the variables for searching
  // ----------------------------------------
  double       Chi2 = 1.0e+18;         // start search with an infinite chi^2
  ColumnVector dist(n); dist(n) = 0.0; // dist(k)=sum_{j=k+1}^{n}(a_j-acond_j)^2/d_j 
  bool         endsearch = false;
  int          count = 0;              // the number of candidates
  
  ColumnVector acond(n); acond(n) = zFlt(n);
  ColumnVector zcond(n); zcond(n) = nint(acond(n));
  double left = acond(n) - zcond(n);
  ColumnVector step(n); step(n) = sign(left);

  // For a very occasional case when the value of float solution zFlt(n) == 0, we
  // compusively give a positive step to continue.
  if (step(n) == 0.0) {
    step(n) = 1;
  }
  
  int    imax = ncands;       // initially, the maximum F(z) is at ncands
  Matrix SS(n, n); SS = 0.0;  // used to compute conditional ambiguities
  
  int k = n;
  
  // Start the main search-loop
  // --------------------------
  while (!endsearch) {
    double newdist = dist(k) + left*left / DD(k);
    if (newdist < Chi2) {
      if (k != 1) { // Case 1: move down
        k = k - 1;
        dist(k)  = newdist;
        for (int j = 1; j <= k; j++) {
          SS(k,j) = SS(k+1,j) + (zcond(k+1)-acond(k+1)) * LL(k+1,j);
        }
        
        acond(k) = zFlt(k) + SS(k, k);
        zcond(k) = round(acond(k));
        left     = acond(k) - zcond(k);
        step(k)  = sign(left);
              
        if (step(k) == 0) {
          step(k) = 1.0;
        }
      }
      else { // Case 2: store the found candidate and try next valid integer
        if (count < ncands - 1) {
          count = count + 1;
          zFix.column(count) = zcond;
          sqnorm(count)        = newdist;
        }           
        else {
          zFix.column(imax) = zcond;
          sqnorm(imax)        = newdist;
          Chi2 = sqnorm.maximum1(imax);
        }
        zcond(1) =  zcond(1) + step(1);  // next valid integer
        left     =  acond(1) - zcond(1);
        step(1)  = -step(1)  - sign(step(1)); 
      }
    }
    else { // Case 3: exit or move up
      if (k == n) {
        endsearch = true;
      }
      else {
        k        =  k + 1;               // move up
        zcond(k) =  zcond(k) + step(k);  // next valid integer
        left     =  acond(k) - zcond(k);
        step(k)  = -step(k)  - sign(step(k));
      }
    }
  }
  
  // Sort
  // ----
  for (int i = 0; i < ncands-1; i++) {
    for (int j = i+1; j < ncands; j++) {
      if (sqnorm[i] > sqnorm[j]) {
        swap(sqnorm[i],sqnorm[j]);
        for (k = 0; k < n; k++) {
          swap(zFix[k][i], zFix[k][j]);
        }
      }
    }
  }
}

#ifdef LAMBDA_MAIN_TEST
// Main test program
// compile: g++ -DLAMBDA_MAIN_TEST -I../../newmat ../../newmat/*.cpp arLambda.cpp
/////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {

  const int nn = 12;

  SymmetricMatrix QQ(nn);
  QQ <<  1.90688560e+04                                                                                                                                                                                                                  
     << -1.57839723e+04 <<  5.90277038e+04                                                                                                                                                                                               
     << -1.73342006e+04 <<  3.81426928e+04 <<  2.81775654e+04                                                                                                                                                                            
     <<  1.44119240e+04 <<  5.62717388e+02 << -7.00050220e+03 <<  1.56055082e+04                                                                                                                                                         
     <<  1.00557170e+04 << -1.38300856e+04 << -1.16958674e+04 <<  5.03970282e+03 <<  6.82077251e+03                                                                                                                                      
     << -1.42592953e+04 <<  2.73734263e+04 <<  2.18861681e+04 << -9.64896531e+03 << -6.88024051e+03 <<  2.32465490e+04                                                                                                                   
     <<  1.48588484e+04 << -1.22991994e+04 << -1.35071695e+04 <<  1.12300704e+04 <<  7.83562345e+03 << -1.11111394e+04 <<  1.15783237e+04                                                                                                
     << -1.22991994e+04 <<  4.59956130e+04 <<  2.97215786e+04 <<  4.38480888e+02 << -1.07766903e+04 <<  2.13299424e+04 << -9.58379157e+03 <<  3.58407377e+04                                                                             
     << -1.35071695e+04 <<  2.97215786e+04 <<  2.19565441e+04 << -5.45493698e+03 << -9.11366311e+03 <<  1.70541567e+04 << -1.05250670e+04 <<  2.31596718e+04 <<  1.71089957e+04                                                          
     <<  1.12300704e+04 <<  4.38480887e+02 << -5.45493698e+03 <<  1.21601359e+04 <<  3.92704096e+03 << -7.51867446e+03 <<  8.75070439e+03 <<  3.41673570e+02 << -4.25060009e+03 <<  9.47543087e+03                                       
     <<  7.83562345e+03 << -1.07766903e+04 << -9.11366311e+03 <<  3.92704096e+03 <<  5.31488728e+03 << -5.36122657e+03 <<  6.10568076e+03 << -8.39742084e+03 << -7.10155552e+03 <<  3.06003207e+03 <<  4.14147091e+03                    
     << -1.11111394e+04 <<  2.13299424e+04 <<  1.70541567e+04 << -7.51867446e+03 << -5.36122657e+03 <<  1.81141936e+04 << -8.65803054e+03 <<  1.66207345e+04 <<  1.32889535e+04 << -5.85870722e+03 << -4.17757899e+03 <<  1.41149564e+04;

  ColumnVector aa(nn);
  aa << -2.84908567e+04
     <<  6.57526299e+04
     <<  3.88303667e+04
     <<  5.00370834e+03
     << -2.91960699e+04
     << -2.97658932e+02
     << -2.22010284e+04
     <<  5.12358375e+04
     <<  3.02577810e+04
     <<  3.89940332e+03
     << -2.27491854e+04
     << -1.59278780e+02;

  ColumnVector    aBie;
  SymmetricMatrix covBie;

  Lambda::search(aa, QQ, aBie, covBie);

  cout.setf(ios::fixed);
  cout << "aFlt = \n"    << setw(10) << setprecision(2) << aa     << endl;
  cout << "aBie = \n"    << setw(10) << setprecision(2) << aBie   << endl;
  cout << "covBie = \n"  << setw( 7) << setprecision(2) << covBie << endl;

  return 0;
}
#endif
