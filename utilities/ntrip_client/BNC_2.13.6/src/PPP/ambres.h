#ifndef AMBRES_H
#define AMBRES_H

#include <iostream>
#include <map>
#include <newmat.h>
#include "pppInclude.h"
#include "pppParlist.h"

namespace BNC_PPP {

  class AmbRes {
  public:
    AmbRes();
    ~AmbRes();

    t_irc run(const bncTime&                  epoTime, 
              const std::vector<t_pppParam*>& params,
              SymmetricMatrix&                QFinal, 
              ColumnVector&                   xFinal,
              double&                         fixRatio,
              std::ostream&                   msg);

  private:
    static const double sigCon;

    class ZdAmb {
    public:
      ZdAmb(const t_pppParam* amb) : _amb(amb), _index(-1) {}
      t_prn             prn() const {return _amb->prn();}
      t_lc              LC() const {return _amb->LC();}
      double            valueApr() const {return _amb->x0();}
      int               index() const {return _index;}
      int               indexGlobal() const {return _amb->indexNew();}
      void              setIndex(int index) {_index = index;}
      bncTime           firstObsTime() const {return _amb->firstObsTime();}
      const t_pppParam* ambGlobal() const {return _amb;}
    private:
      const t_pppParam* _amb;
      int               _index;
    };
  
    class SdAmb {
    public:
      SdAmb(const ZdAmb& zdAmb1, const ZdAmb& zdAmb2,
            const ColumnVector& xBie, const SymmetricMatrix& covBie);
      t_lc              _LC;
      t_prn             _prn1;
      t_prn             _prn2;
      double            _aprVal;
      double            _bieValue;
      double            _bieSigma;
      bool              _fixed;
      bncTime           _firstObsTime;
      const t_pppParam* _ambGlobal[2];
    };
  
    class AmbGroup {
    public:
      AmbGroup(char sys, const t_lc& LC) : _system(sys), _LC(LC), _zdAmbRef(0) {};
      char               _system;
      t_lc               _LC;
      const ZdAmb*       _zdAmbRef;
      std::vector<ZdAmb> _zdAmbs;
      std::vector<SdAmb> _sdAmbs;
    };

    static bool isResolvable(const t_pppParam* amb);
    static bool isFixable(double xx, double sigma, bool* checked = 0);

    void reset() {
      _numZdAmbs = 0;
      _ambGroups.clear();
      _QQ.cleanup();
      _xx.cleanup();
    }

    void addToGroup(const t_pppParam* amb);
    void selectReference(AmbGroup& ambGroup);
    void printBieResults(const ColumnVector& xBie, const SymmetricMatrix& covBie,
                         std::ostream& msg) const;
    void setConstraints(SymmetricMatrix& QFinal, ColumnVector& xFinal,
                        const ColumnVector& xBie, const SymmetricMatrix& covBie, 
                        double& fixRatio, std::ostream& msg);

    unsigned              _numZdAmbs;
    std::vector<AmbGroup> _ambGroups;
    SymmetricMatrix       _QQ;
    ColumnVector          _xx;

  };

}

#endif
