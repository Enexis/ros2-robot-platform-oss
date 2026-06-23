/*
 * bncrawuploadcaster.cpp
 *
 *  Created on: Aug 26, 2025
 *      Author: stuerze
 */

/* -------------------------------------------------------------------------
 * BKG NTRIP Server
 * -------------------------------------------------------------------------
 *
 * Class:      bncRawUploadCaster
 *
 * Purpose:    Connection to NTRIP Caster for Ephemeris
 *
 * Author:     L. Mervart
 *
 * Created:    03-Apr-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>
#include <math.h>
#include "bncrawuploadcaster.h"
#include "bncsettings.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncRawUploadCaster::bncRawUploadCaster() {
  bncSettings settings;

  // List of upload casters
  // ----------------------
  int iRow = -1;
  QListIterator<QString> it(settings.value("uploadRawMountpointsOut").toStringList());
  while (it.hasNext()) {
    QStringList hlp = it.next().split(",");
    if (hlp.size() > 5) {
      ++iRow;
      QString sourceMount = hlp[0];
      int  outPort = hlp[2].toInt();
      bncUploadCaster* newCaster = new bncUploadCaster(hlp[3], hlp[1], outPort,
                                                       hlp[4], hlp[5],
                                                       hlp[6], iRow, 0);

      connect(newCaster, SIGNAL(newBytes(QByteArray,double)),
              this, SIGNAL(newBytes(QByteArray,double)));

      newCaster->start();
      _casters[sourceMount] = newCaster;
    }
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncRawUploadCaster::~bncRawUploadCaster() {
  QMapIterator<QString,bncUploadCaster*>it(_casters);
  while (it.hasNext()) {
    it.next();
    it.value()->deleteSafely();
  }
  _casters.clear();
}

// Output into the raw upload socket
////////////////////////////////////////////////////////////////////////////
void bncRawUploadCaster::slotNewRawData(QByteArray staID, QByteArray data) {

    if (_casters.contains(QString(staID))) {
      _casters[QString(staID)]->setOutBuffer(data);
    }
}
