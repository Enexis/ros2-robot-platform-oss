/*
 * bncrawuploadcaster.h
 *
 *  Created on: Aug 26, 2025
 *      Author: stuerze
 */

#ifndef SRC_UPLOAD_BNCRAWUPLOADCASTER_H_
#define SRC_UPLOAD_BNCRAWUPLOADCASTER_H_

#include "bncuploadcaster.h"
#include "bncsettings.h"

class bncRawUploadCaster : public QObject {
 Q_OBJECT
 public:
  bncRawUploadCaster();
  virtual ~bncRawUploadCaster();
 signals:
  void newBytes(QByteArray staID, double nbyte);
  public slots:
  void slotNewRawData(QByteArray staID, QByteArray data);
 private:
  QMap<QString, bncUploadCaster*> _casters;
};

#endif /* SRC_UPLOAD_BNCRAWUPLOADCASTER_H_ */
