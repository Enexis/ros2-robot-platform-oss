#ifndef LAMBDA_H
#define LAMBDA_H

#include <newmat.h>

namespace BNC_PPP {

  class Lambda {
  public:
    static void search(ColumnVector aFlt, const SymmetricMatrix& QaFlt,
                       ColumnVector& aFix, SymmetricMatrix& covBie);

  private:
    class Info {
    public:
      Matrix       aFix;
      Matrix       zFix;
      ColumnVector zBie;
      ColumnVector wgt;
    };

    static double erf(double xx);

    static double normcdf(double xx, double mu = 0.0, double sigma = 1.0);

    static void ldldecom(const SymmetricMatrix& QQ, LowerTriangularMatrix& LL, DiagonalMatrix& DD);

    static void decorrel(const SymmetricMatrix& QaFlt, const ColumnVector& aFlt, 
                         SymmetricMatrix& QzFlt, Matrix& ZZ, LowerTriangularMatrix& LL, 
                         DiagonalMatrix& DD, ColumnVector& zFlt, Matrix& iZt);

    static void ssearch(const ColumnVector& zFlt, const LowerTriangularMatrix& LL, 
                        const DiagonalMatrix& DD, int ncands, Matrix& zFix, ColumnVector& sqnorm);

    static inline double nint(double val);

    static inline void swap(double& a, double& b);

    static inline double sign(double a);

    static void BIE(const ColumnVector& zFlt, const LowerTriangularMatrix& LL, 
                    const DiagonalMatrix& DD, Info& info);
  };

}

#endif
