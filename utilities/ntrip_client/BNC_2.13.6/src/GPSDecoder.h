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

#ifndef GPSDECODER_H
#define GPSDECODER_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

#include <QtCore>

#include "bncconst.h"
#include "bnctime.h"
#include "satObs.h"
#include "crs.h"

class bncRinex;
using namespace std;

class GPSDecoder {
 public:
  GPSDecoder();
  virtual ~GPSDecoder();

  virtual t_irc Decode(char* buffer, int bufLen,
                       std::vector<std::string>& errmsg) = 0;


  virtual int corrGPSEpochTime() const {return -1;}

  void initRinex(const QByteArray& staID, const QUrl& mountPoint,
                 const QByteArray& latitude, const QByteArray& longitude,
                 const QByteArray& nmea, const QByteArray& ntripVersion);

  void dumpRinexEpoch(const t_satObs& obs, const QByteArray& format);

  void setRinexReconnectFlag(bool flag);

  struct t_typeInfo {
    t_typeInfo() {
      _type = 0;
      _size = 0;
    };
    int    _type;
    size_t _size;
  };


  struct t_antRefPoint {
    enum t_type { ARP, APC };

    t_antRefPoint() {
      _xx = _yy = _zz = _height = 0.0;
      _type = ARP;
      _height_f = false;
      _message  = 0;
    };

    double _xx;
    double _yy;
    double _zz;
    t_type _type;
    double _height;
    bool   _height_f;
    int    _message;
  };

  struct t_antInfo {
    t_antInfo() {
    };
    char _descriptor[256];
    char _serialnumber[256];
  };

  struct t_recInfo {
    t_recInfo() {
    };
    char _descriptor[256];
    char _serialnumber[256];
    char _firmware[256];
  };

  class t_GloBiasInfo {
  public:
    t_GloBiasInfo() {
      init();
    };
    bool operator!=(const t_GloBiasInfo& gloBiasInfo2) {

      if (_staID     != gloBiasInfo2._staID     ||
          _indicator != gloBiasInfo2._indicator ||
          (fabs(_L1C_value - gloBiasInfo2._L1C_value) > 0.000000000001 ) ||
          (fabs(_L1P_value - gloBiasInfo2._L1P_value) > 0.000000000001 ) ||
          (fabs(_L2C_value - gloBiasInfo2._L2C_value) > 0.000000000001 ) ||
          (fabs(_L2P_value - gloBiasInfo2._L2P_value) > 0.000000000001 )  ) {
        setChanged(true);
        return true;
      }
      else {
        setChanged(false);
        return false;
      }
    }

    void set(const t_GloBiasInfo& gloBiasInfo) {
      _staID     = gloBiasInfo._staID;
      _indicator = gloBiasInfo._indicator;
      _L1C_value = gloBiasInfo._L1C_value;
      _L1P_value = gloBiasInfo._L1P_value;
      _L2C_value = gloBiasInfo._L2C_value;
      _L2P_value = gloBiasInfo._L2P_value;
    }
    void setChanged(bool changed) {
      _changed = changed;
    }
    bool changed() {return _changed;};
    void init() {
      _staID = 0;
      _indicator = 1;
      _L1C_value = 0.0;
      _L1P_value = 0.0;
      _L2C_value = 0.0;
      _L2P_value = 0.0;
      _changed = false;
    }
    QString toString() {
      QString biasIndicator =  (_indicator == 1) ? QString("aligned") : QString("unaligned");
      QString biasesStr =
          QString(": GLONASS L1/L2 Code-Phase Biases: staID=%1 indicator=%2 L1C=%3 L1P=%4 L2C=%5 L2P=%6")
          .arg(_staID).arg(biasIndicator)
          .arg(_L1C_value, 0, 'f', 2)
          .arg(_L1P_value, 0, 'f', 2)
          .arg(_L2C_value, 0, 'f', 2)
          .arg(_L2P_value, 0, 'f', 2);

      return biasesStr;
    }

    int      _staID;
    int      _indicator;
    double   _L1C_value;
    double   _L1P_value;
    double   _L2C_value;
    double   _L2P_value;
    bool     _changed;


  };

  /** List of observations */
  QList<t_satObs>         _obsList;
  QList<t_typeInfo>       _typeList;           // RTCM message type as message size
  QList<t_antInfo>        _antType;            // RTCM antenna descriptor
  QList<t_recInfo>        _recType;            // RTCM receiver descriptor
  QList<t_antRefPoint>    _antList;            // RTCM antenna XYZ
  QList<t_helmertPar>     _helmertPar;         // List of Helmert parameter sets
  QList<t_serviceCrs>     _serviceCrs;         // Service CRS
  QList<t_rtcmCrs>        _rtcmCrs;            // RTCM CRS
  QString                 _gloFrq;             // GLONASS slot
  t_GloBiasInfo           _gloBiasInfo;          // RTCM GLO bias information message
  bncRinex*               _rnx;                // RINEX writer
};

#endif
