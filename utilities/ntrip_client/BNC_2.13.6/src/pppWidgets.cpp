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
 * Class:      t_pppWidgets
 *
 * Purpose:    This class stores widgets for PPP options
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>

#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>

#include "pppWidgets.h"
#include "qtfilechooser.h"
#include "bncsettings.h"
#include "bnccore.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppWidgets::t_pppWidgets() {

  _dataSource   = new QComboBox();     _dataSource  ->setObjectName("PPP/dataSource");   _widgets << _dataSource;
  _rinexObs     = new qtFileChooser(); _rinexObs    ->setObjectName("PPP/rinexObs");     _widgets << _rinexObs;
  _rinexNav     = new qtFileChooser(); _rinexNav    ->setObjectName("PPP/rinexNav");     _widgets << _rinexNav;
  _corrMount    = new QLineEdit();     _corrMount   ->setObjectName("PPP/corrMount");    _widgets << _corrMount;
  _biasMount    = new QLineEdit();     _biasMount   ->setObjectName("PPP/biasMount");    _widgets << _biasMount;
  _ionoMount    = new QLineEdit();     _ionoMount   ->setObjectName("PPP/ionoMount");    _widgets << _ionoMount;
  _corrFile     = new qtFileChooser(); _corrFile    ->setObjectName("PPP/corrFile");     _widgets << _corrFile;
  _biasFile     = new qtFileChooser(); _biasFile    ->setObjectName("PPP/biasFile");     _widgets << _biasFile;
  _ionoFile     = new qtFileChooser(); _ionoFile    ->setObjectName("PPP/ionoFile");     _widgets << _ionoFile;
  _crdFile      = new qtFileChooser(); _crdFile     ->setObjectName("PPP/crdFile");      _widgets << _crdFile;
  _antexFile    = new qtFileChooser(); _antexFile   ->setObjectName("PPP/antexFile");    _widgets << _antexFile;
  _blqFile      = new qtFileChooser(); _blqFile     ->setObjectName("PPP/blqFile");      _widgets << _blqFile;
  _logPath      = new QLineEdit();     _logPath     ->setObjectName("PPP/logPath");      _widgets << _logPath;
  _logMode      = new QComboBox();     _logMode     ->setObjectName("PPP/logMode");      _widgets << _logMode;
  _nmeaPath     = new QLineEdit();     _nmeaPath    ->setObjectName("PPP/nmeaPath");     _widgets << _nmeaPath;
  _snxtroPath   = new QLineEdit();     _snxtroPath  ->setObjectName("PPP/snxtroPath");   _widgets << _snxtroPath;
  _snxtroSampl  = new QComboBox();     _snxtroSampl ->setObjectName("PPP/snxtroSampl");  _widgets << _snxtroSampl;
  _snxtroIntr   = new QComboBox();     _snxtroIntr  ->setObjectName("PPP/snxtroIntr");   _widgets << _snxtroIntr;
  _snxtroAc     = new QLineEdit();     _snxtroAc    ->setObjectName("PPP/snxtroAc");     _widgets << _snxtroAc;
  _snxtroSolId  = new QLineEdit();     _snxtroSolId ->setObjectName("PPP/snxtroSolId");  _widgets << _snxtroSolId;
  _snxtroSolType= new QLineEdit();     _snxtroSolType->setObjectName("PPP/snxtroSolType");_widgets << _snxtroSolType;
  _snxtroCampId = new QLineEdit();     _snxtroCampId ->setObjectName("PPP/snxtroCampId");_widgets << _snxtroCampId;
  _staTable     = new QTableWidget();  _staTable    ->setObjectName("PPP/staTable");     _widgets << _staTable;
  _lcGPS        = new QComboBox();     _lcGPS       ->setObjectName("PPP/lcGPS");        _widgets << _lcGPS;
  _lcGLONASS    = new QComboBox();     _lcGLONASS   ->setObjectName("PPP/lcGLONASS");    _widgets << _lcGLONASS;
  _lcGalileo    = new QComboBox();     _lcGalileo   ->setObjectName("PPP/lcGalileo");    _widgets << _lcGalileo;
  _lcBDS        = new QComboBox();     _lcBDS       ->setObjectName("PPP/lcBDS");        _widgets << _lcBDS;
  _constraints  = new QComboBox();     _constraints ->setObjectName("PPP/constraints");  _widgets << _constraints;
  _sigmaC1      = new QLineEdit();     _sigmaC1     ->setObjectName("PPP/sigmaC1");      _widgets << _sigmaC1;
  _sigmaL1      = new QLineEdit();     _sigmaL1     ->setObjectName("PPP/sigmaL1");      _widgets << _sigmaL1;
  _sigmaGIM     = new QLineEdit();     _sigmaGIM    ->setObjectName("PPP/sigmaGIM");     _widgets << _sigmaGIM;
  _maxResC1     = new QLineEdit();     _maxResC1    ->setObjectName("PPP/maxResC1");     _widgets << _maxResC1;
  _maxResL1     = new QLineEdit();     _maxResL1    ->setObjectName("PPP/maxResL1");     _widgets << _maxResL1;
  _maxResGIM    = new QLineEdit();     _maxResGIM   ->setObjectName("PPP/maxResGIM");    _widgets << _maxResGIM;
  _minObs       = new QSpinBox();      _minObs      ->setObjectName("PPP/minObs");       _widgets << _minObs;
  _minEle       = new QSpinBox();      _minEle      ->setObjectName("PPP/minEle");       _widgets << _minEle;
  _eleWgtCode   = new QCheckBox();     _eleWgtCode  ->setObjectName("PPP/eleWgtCode");   _widgets << _eleWgtCode;
  _eleWgtPhase  = new QCheckBox();     _eleWgtPhase ->setObjectName("PPP/eleWgtPhase");  _widgets << _eleWgtPhase;
  _seedingTime  = new QLineEdit();     _seedingTime ->setObjectName("PPP/seedingTime");  _widgets << _seedingTime;
  _corrWaitTime = new QSpinBox();      _corrWaitTime->setObjectName("PPP/corrWaitTime"); _widgets << _corrWaitTime;

  _arGPS        = new QCheckBox();     _arGPS       ->setObjectName("PPP/arGPS");        _widgets << _arGPS;
  _arGalileo    = new QCheckBox();     _arGalileo   ->setObjectName("PPP/arGalileo");    _widgets << _arGalileo;
  _arBDS        = new QCheckBox();     _arBDS       ->setObjectName("PPP/arBDS");        _widgets << _arBDS;
  _arMinNumEpo  = new QSpinBox();      _arMinNumEpo ->setObjectName("PPP/arMinNumEpo");  _widgets << _arMinNumEpo;
  _arMinNumSat  = new QSpinBox();      _arMinNumSat ->setObjectName("PPP/arMinNumSat");  _widgets << _arMinNumSat;
  _arUseYaw     = new QCheckBox();     _arUseYaw    ->setObjectName("PPP/arUseYaw");     _widgets << _arUseYaw;
  _arMaxFrac    = new QLineEdit();     _arMaxFrac   ->setObjectName("PPP/arMaxFrac");    _widgets << _arMaxFrac;
  _arMaxSig     = new QLineEdit();     _arMaxSig    ->setObjectName("PPP/arMaxSig");     _widgets << _arMaxSig;

  _addStaButton = new QPushButton("Add Station");    _widgets << _addStaButton;
  _delStaButton = new QPushButton("Delete Station"); _widgets << _delStaButton;

  _addStaButton->setWhatsThis(tr("<p>Hit the 'Add Station' button to add a new line to the Station table.</p>"));
  _delStaButton->setWhatsThis(tr("<p>Hit the 'Delete Station' button to delete a highlighted row from the Station table.</p>"));

  _plotCoordinates  = new QLineEdit;    _plotCoordinates ->setObjectName("PPP/plotCoordinates");  _widgets << _plotCoordinates;
  _mapWinButton     = new QPushButton;  _mapWinButton    ->setObjectName("PPP/mapWinButton");     _widgets << _mapWinButton;
  _audioResponse    = new QLineEdit;    _audioResponse   ->setObjectName("PPP/audioResponse");    _widgets << _audioResponse;
  _mapWinDotSize    = new QLineEdit;    _mapWinDotSize   ->setObjectName("PPP/mapWinDotSize");    _widgets << _mapWinDotSize;
  _mapWinDotColor   = new QComboBox;    _mapWinDotColor  ->setObjectName("PPP/mapWinDotColor");   _widgets << _mapWinDotColor;
  _mapSpeedSlider   = new QSlider;      _mapSpeedSlider  ->setObjectName("PPP/mapSpeedSlider");   _widgets << _mapSpeedSlider;

  _dataSource->setEditable(false);
  _dataSource->addItems(QString(",Real-Time Streams,RINEX Files").split(","));
  connect(_dataSource, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slotEnableWidgets()));
  connect(_constraints, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slotEnableWidgets()));
  connect(_arGPS, SIGNAL(stateChanged(int)),  this, SLOT(slotEnableWidgets()));
  connect(_arGalileo, SIGNAL(stateChanged(int)),  this, SLOT(slotEnableWidgets()));
  connect(_arBDS, SIGNAL(stateChanged(int)),  this, SLOT(slotEnableWidgets()));
  connect(_snxtroPath, SIGNAL(textChanged(const QString &)), this, SLOT(slotPPPTextChanged()));
  connect(_snxtroAc, SIGNAL(textChanged(const QString &)), this, SLOT(slotPPPTextChanged()));
  connect(_snxtroSolId, SIGNAL(textChanged(const QString &)), this, SLOT(slotPPPTextChanged()));
  connect(_snxtroSolType, SIGNAL(textChanged(const QString &)), this, SLOT(slotPPPTextChanged()));
  connect(_snxtroCampId, SIGNAL(textChanged(const QString &)), this, SLOT(slotPPPTextChanged()));

  _logMode->setEditable(false);
  _logMode->addItems(QString("normal,debug,all").split(","));

  slotEnableWidgets();

  _lcGPS->setEditable(true);
  _lcGPS->addItems(QString("Pi&Li,Pi,P1&L1,P1,P3&L3,P3,L3,no,P125&L125").split(","));

  _lcGLONASS->setEditable(false);
  _lcGLONASS->addItems(QString("Pi&Li,Pi,P1&L1,P1,P3&L3,P3,L3,no").split(","));

  _lcGalileo->setEditable(true);
  _lcGalileo->addItems(QString("Pi&Li,Pi,P1&L1,P1,P3&L3,P3,L3,no,P1576&L1576").split(","));

  _lcBDS->setEditable(true);
  _lcBDS->addItems(QString("Pi&Li,Pi,P1&L1,P1,P3&L3,P3,L3,no,P1576&L1576").split(","));

  _constraints->setEditable(false);
  _constraints->addItems(QString("no,Ionosphere: pseudo-obs").split(","));

  _snxtroSampl->setEditable(false);
  _snxtroSampl->addItems(QString("1 sec,5 sec,10 sec,30 sec,60 sec,300 sec").split(","));

  _snxtroIntr->setEditable(false);
  _snxtroIntr->addItems(QString("1 min,2 min,5 min,10 min,15 min,30 min,1 hour,1 day").split(","));
  _snxtroIntr->setCurrentIndex(6);

  _minObs->setMinimum(4);
  _minObs->setMaximum(6);
  _minObs->setSingleStep(1);

  _minEle->setMinimum(0);
  _minEle->setMaximum(20);
  _minEle->setSingleStep(1);
  _minEle->setSuffix(" deg");

  _arMinNumEpo->setMinimum(5);
  _arMinNumEpo->setMaximum(60);
  _arMinNumEpo->setSingleStep(5);

  _arMinNumSat->setMinimum(4);
  _arMinNumSat->setMaximum(8);
  _arMinNumSat->setSingleStep(1);

  _corrWaitTime->setMinimum(0);
  _corrWaitTime->setMaximum(20);
  _corrWaitTime->setSingleStep(1);
  _corrWaitTime->setSuffix(" sec");

  _staTable->setColumnCount(11);
  _staTable->setRowCount(0);
  _staTable->setHorizontalHeaderLabels(
  QString("Station,Sigma N,Sigma E,Sigma H,Noise N,Noise E,Noise H,Tropo Sigma,Tropo Noise, NMEA Port,Signal Priorities").split(","));
  _staTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
  _staTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  _staTable->setColumnWidth(0,120);
  _staTable->setColumnWidth(10,220);
#if QT_VERSION >= 0x050000
  _staTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
#endif
  _staTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  connect(_addStaButton, SIGNAL(clicked()), this, SLOT(slotAddStation()));
  connect(_delStaButton, SIGNAL(clicked()), this, SLOT(slotDelStation()));

  _mapWinButton->setText("Open Map");

  _mapWinDotColor->setEditable(false);
  _mapWinDotColor->addItems(QString("red,yellow").split(","));

  _mapSpeedSlider->setOrientation(Qt::Horizontal);
  _mapSpeedSlider->setRange(1, 100);
  _mapSpeedSlider->setTickPosition(QSlider::TicksBelow);
  _mapSpeedSlider->setTickInterval(10);
  connect(_mapSpeedSlider, SIGNAL(valueChanged(int)), BNC_CORE, SIGNAL(mapSpeedSliderChanged(int)));

  // WhatsThis, PPP (2)
  // ------------------
  _corrWaitTime->setWhatsThis(tr("<p>Zero value means that BNC processes each epoch of data immediately after its arrival using satellite clock corrections available at that time.</p><p> Specifying a non-zero value (i.e. 5 sec) means that the epochs of data are buffered and the processing of each epoch is postponed till the satellite clock corrections not older than '5 sec' (example) become available. <i>[key: PPP/corrWaitTime]</i><p>"));
  _seedingTime->setWhatsThis(tr("<p>Enter the length of a startup period in seconds for which you want to fix the PPP solutions to known a priori coordinates as introduced through option 'Coordinates file'. Adjust 'Sigma N/E/H' in the PPP Stations table according to the coordinate's precision. Fixing a priori coordinates is done in BNC through setting 'Noise N/E/H' temporarily to zero.</p><p>This option allows the PPP solution to rapidly converge. It requires that the antenna remains unmoved on the a priori known position throughout the startup period.</p><p>A value of 60 is likely to be an appropriate choice.</p><p>Default is an empty option field, meaning that you don't want BNC to fix PPP solutions during startup to an a priori coordinate. <i>[key: PPP/seedingTime]</i></p>"));

  // WhatsThis, PPP (3)
  // ------------------
  _staTable->setWhatsThis(tr("<p>Specify values for Sigma and white Noise of the Stations North, East and Height coordinate components in meters. Specify also a Sigma in meters for a priori model based Tropospheric delays and a Sigma in meters per second for the delay's Noise. You can also specify a 'NMEA Port' to output coordinates in NMEA format through an IP port of your local host. Specify a list of signal priorities for the observations that shall be used for PPP.</p>"
                             "<p>'Sigma' is meant to describe the uncertainty of a single coordinate or tropospheric delay estimated for one epoch. 'Noise' is meant to describe the variation of estimates from epoch to epoch.</p><p><ul><li>A Sigma of 100.0 meters may be an appropriate choice e.g. for the initial N/E/H coordinates. However, this value may be significantly smaller (i.e. 0.01) for stations with well-known a priori coordinates.</li><li>A Noise of 100.0 meters for the estimated N/E/H coordinates may also be appropriate considering the potential movement of a rover position.</li><li>A value of 0.1 meters may be an appropriate Sigma for the a priori model based Tropospheric delay estimation.</li><li>Specify a Noise to describe the expected variation of the tropospheric effect over time. Supposing 1Hz observation data, specifying a value of 3e-6 would mean that the tropospheric effect may vary 3600 * 3e-6 = 0.01 meters per hour.</li></ul></p>"
                             "<p>'Signal Priorities' can be specified as system (G,R,E,C) and frequency specific. Two frequency bands per GNSS are allowed and will be considered. The following frequency bands are available for selection: <ul>"
                             "<li>G: 1, 2, 5</li>"
                             "<li>R: 1, 2</li>"
                             "<li>E: 1, 5, 6, 7, 8</li>"
                             "<li>C: 1, 2, 5, 6, 7, 8</li>"
                             "</ul>"
                             "<p>'Default is the following list of 'Signal Priorities': <ul><li>'G:12&CWPSLX R:12&CP E:1&CBX E:5&QIX C:26&IQX'</li></ul>"
                             "<p>But it is recommended to specify it in more detail per individual station, e.g.:</p> <ul>  <li>'G:12&W R:12&P E:1&C E:5&Q C:26&I'</li></ul> "
                             "<p> <i>[key: PPP/staTable]</i></p>"));

  // WhatsThis, PPP (4)
  // ------------------
  _plotCoordinates->setWhatsThis(tr("<p>For one of your PPP Stations BNC can produce a time series plot of coordinate displacements in the 'PPP Plot' tab below. Specify a 'Mountpoint' (when in 'Real-Time Streams' mode) or the 9/4-character station ID (when in 'RINEX Files' mode) to define the station whose coordinate displacements you would like to see plotted.</p><p>Note that this option makes only sense for a stationary receiver with known a priori marker coordinates as specified through PPP option 'Coordinates file'.</p><p>Default is an empty option field, meaning that BNC shall not produce a time series plot of PPP coordinate displacements. <i>[key: PPP/plotCoordinates]</i></p>"));
  _audioResponse->setWhatsThis(tr("<p>Specify an 'Audio response' threshold in meters. A beep is produced by BNC whenever a horizontal PPP coordinate component differs by more than the threshold value from the a priori marker coordinate.</p><p>Default is an empty option field, meaning that you don't want BNC to produce alarm signals. <i>[key: PPP/audioResponse]</i></p>"));
  _mapWinButton->setWhatsThis(tr("<p>You may like to track your rover position using Open Street Map as a background map. A 'Track map' can be produced with BNC in 'Real-Time Streams' or 'RINEX files' PPP mode.</p><p>The 'Open Map' button opens a windows showing a map according to specified options.</p><p>Even in 'RINEX files' post processing mode you should not forget to specify a proxy under the 'Network' tab if that is operated in front of BNC because the program needs to download the map data. Without any entry, BNC will try to use the system proxies.</p>"));
  _mapWinDotSize->setWhatsThis(tr("<p>Specify the size of dots showing rover positions on the track map.</p><p>A dot size of '3' may be appropriate. The maximum possible dot size is '10'. An empty option field or a size of '0' would mean that you don't want BNC to show the rover's track on the map. <i>[key: PPP/mapWinDotSize]</i></p>"));
  _mapWinDotColor->setWhatsThis(tr("<p>Specify the color of dots showing the rover track on the map. <i>[key: PPP/mapWinDotColor]</i></p>"));
  _mapSpeedSlider->setWhatsThis(tr("<p>With BNC in 'RINEX files' PPP post processing mode you can specify the speed of computations as appropriate for 'Track map' visualization.</p><p>Note that you can adjust 'Post-processing speed' on-the-fly while BNC is already processing your observations. <i>[key: PPP/mapSpeedSlider]</i></p>"));

  readOptions();
}

//
////////////////////////////////////////////////////////////////////////////
t_pppWidgets::~t_pppWidgets() {
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppWidgets::readOptions() {

  bncSettings settings;

  auto setWidgetValue = [settings](QWidget* widget, QString defValue = QString()) {
    if      (auto* obj = qobject_cast<QLineEdit*>(widget)) {
      QString text = settings.value(obj->objectName()).toString();
      if (text.isEmpty() && !defValue.isEmpty()) {
        text = defValue;
      }
      obj->setText(text);
    }
    else if (auto* obj = qobject_cast<QComboBox*>(widget)) {
      QString text = settings.value(obj->objectName()).toString();
      int ii = obj->findText(text);
      if (ii != -1) {
        obj->setCurrentIndex(ii);
      }
      else if (obj->isEditable()) {
        obj->insertItem(0, text);
        obj->setCurrentIndex(0);
      }
    }
    else if (auto* obj = qobject_cast<qtFileChooser*>(widget)) {
      obj->setFileName(settings.value(obj->objectName()).toString());
    }
    else if (auto* obj = qobject_cast<QCheckBox*>(widget)) {
      obj->setCheckState(Qt::CheckState(settings.value(obj->objectName()).toInt()));
    }
    else if (auto* obj = qobject_cast<QSpinBox*>(widget)) {
      obj->setValue(settings.value(obj->objectName()).toInt());
    }
    else if (auto* obj = qobject_cast<QSlider*>(widget)) {
      int value = settings.value(obj->objectName()).toInt();
      if (value == 0) value = obj->maximum();
      obj->setSliderPosition(value);
    }
  };

  QListIterator<QWidget*> iw(_widgets);
  while (iw.hasNext()) {
    setWidgetValue(iw.next());
  }

  // Set default values for some widgets
  // -----------------------------------
  setWidgetValue(_sigmaC1,     "1.0");
  setWidgetValue(_sigmaL1,     "0.01");
  setWidgetValue(_sigmaGIM,    "1.0");
  setWidgetValue(_maxResC1,    "3.0");
  setWidgetValue(_maxResL1,    "0.03");
  setWidgetValue(_maxResGIM,   "3.0");
  setWidgetValue(_seedingTime, "0");

  // Table with stations
  // -------------------
  for (int iRow = _staTable->rowCount()-1; iRow >=0; iRow--) {
    _staTable->removeRow(iRow);
  }
  int iRow = -1;
  QListIterator<QString> it(settings.value(_staTable->objectName()).toStringList());
  while (it.hasNext()) {
    QStringList hlp = it.next().split(",");
    ++iRow;
    _staTable->insertRow(iRow);
    for (int iCol = 0; iCol < hlp.size(); iCol++) {
      _staTable->setItem(iRow, iCol, new QTableWidgetItem(hlp[iCol]));
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppWidgets::saveOptions() {

  bncSettings settings;

  auto storeWidgetValue = [&settings](QWidget* widget) {
    if      (auto* obj = qobject_cast<QLineEdit*>(widget)) {
      settings.setValue(obj->objectName(), obj->text());
    }
    else if (auto* obj = qobject_cast<QComboBox*>(widget)) {
      settings.setValue(obj->objectName(), obj->currentText());
    }
    else if (auto* obj = qobject_cast<qtFileChooser*>(widget)) {
      settings.setValue(obj->objectName(), obj->fileName());
    }
    else if (auto* obj = qobject_cast<QCheckBox*>(widget)) {
      settings.setValue(obj->objectName(), obj->checkState());
    }
    else if (auto* obj = qobject_cast<QSpinBox*>(widget)) {
      settings.setValue(obj->objectName(), obj->value());
    }
    else if (auto* obj = qobject_cast<QSlider*>(widget)) {
      settings.setValue(obj->objectName(), obj->value());
    }
  };

  QListIterator<QWidget*> it(_widgets);
  while (it.hasNext()) {
    storeWidgetValue(it.next());
  }

  QStringList staList;
  for (int iRow = 0; iRow < _staTable->rowCount(); iRow++) {
    QString hlp;
    for (int iCol = 0; iCol < _staTable->columnCount(); iCol++) {
      if (_staTable->item(iRow, iCol)) {
        hlp += _staTable->item(iRow, iCol)->text() + ",";
      }
    }
    if (!hlp.isEmpty()) {
      staList << hlp;
    }
  }
  settings.setValue(_staTable->objectName(), staList);
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppWidgets::slotEnableWidgets() {

  const static QPalette paletteWhite(QColor(255, 255, 255));
  const static QPalette paletteGray(QColor(230, 230, 230));

  bool allDisabled   = _dataSource->currentText() == "";
  bool realTime      = _dataSource->currentText() == "Real-Time Streams";
  bool rinexFiles    = _dataSource->currentText() == "RINEX Files";
  bool pseudoObsIono = _constraints->currentText() == "Ionosphere: pseudo-obs";

  QListIterator<QWidget*> it(_widgets);
  while (it.hasNext()) {
    QWidget* widget = it.next();
    widget->setEnabled(!allDisabled);
  }

  if      (realTime) {
    _rinexObs->setEnabled(false);
    _rinexNav->setEnabled(false);
    _corrFile->setEnabled(false);
    _biasFile->setEnabled(false);
    _ionoFile->setEnabled(false);
  }
  else if (rinexFiles) {
    _corrMount    ->setEnabled(false);
    _biasMount    ->setEnabled(false);
    _ionoMount    ->setEnabled(false);
    _audioResponse->setEnabled(false);
  }

  if ( _snxtroPath->text() != "" && !allDisabled) {
    _snxtroSampl  ->setEnabled(true);
    _snxtroIntr   ->setEnabled(true);
    _snxtroAc     ->setEnabled(true);
    _snxtroSolId  ->setEnabled(true);
    _snxtroSolType->setEnabled(true);
    _snxtroCampId ->setEnabled(true);
  }
  else {
    _snxtroSampl  ->setEnabled(false);
    _snxtroIntr   ->setEnabled(false);
    _snxtroAc     ->setEnabled(false);
    _snxtroSolId  ->setEnabled(false);
    _snxtroSolType->setEnabled(false);
    _snxtroCampId ->setEnabled(false);
  }


  if (pseudoObsIono) {
   _sigmaGIM->setEnabled(true);
   _maxResGIM->setEnabled(true);
  } else {
    _sigmaGIM->setEnabled(false);
    _maxResGIM->setEnabled(false);
  }

  bool ar = (_arGPS->checkState()     == Qt::Checked ||
             _arGalileo->checkState() == Qt::Checked ||
             _arBDS->checkState()     == Qt::Checked);
  _arMinNumEpo->setEnabled(ar);
  _arMinNumSat->setEnabled(ar);
  _arUseYaw   ->setEnabled(true);
  _arMaxFrac  ->setEnabled(ar);
  _arMaxSig   ->setEnabled(ar);

  _dataSource->setEnabled(true);

  it.toFront();
  while (it.hasNext()) {
    QWidget* widget = it.next();
    if (widget->isEnabled()) {
      widget->setPalette(paletteWhite);
    }
    else {
      widget->setPalette(paletteGray);
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppWidgets::slotAddStation() {
  int iRow = _staTable->rowCount();
  _staTable->insertRow(iRow);
  QString preferredAttributes = "G:12&CWPSLX R:12&CP E:1&CBX E:5&QIX C:26&IQX";

  for (int iCol = 0; iCol < _staTable->columnCount(); iCol++) {
     if (iCol ==  0) _staTable->setItem(iRow, iCol, new QTableWidgetItem(""));
     if (iCol ==  1) _staTable->setItem(iRow, iCol, new QTableWidgetItem("100.0"));
     if (iCol ==  2) _staTable->setItem(iRow, iCol, new QTableWidgetItem("100.0"));
     if (iCol ==  3) _staTable->setItem(iRow, iCol, new QTableWidgetItem("100.0"));
     if (iCol ==  4) _staTable->setItem(iRow, iCol, new QTableWidgetItem("100.0"));
     if (iCol ==  5) _staTable->setItem(iRow, iCol, new QTableWidgetItem("100.0"));
     if (iCol ==  6) _staTable->setItem(iRow, iCol, new QTableWidgetItem("100.0"));
     if (iCol ==  7) _staTable->setItem(iRow, iCol, new QTableWidgetItem("0.1"));
     if (iCol ==  8) _staTable->setItem(iRow, iCol, new QTableWidgetItem("3e-6"));
     if (iCol ==  9) _staTable->setItem(iRow, iCol, new QTableWidgetItem("0"));
     if (iCol == 10) _staTable->setItem(iRow, iCol, new QTableWidgetItem(preferredAttributes));
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppWidgets::slotDelStation() {
  int nRows = _staTable->rowCount();
  std::vector <bool> flg(nRows);
  for (int iRow = 0; iRow < nRows; iRow++) {
    if (_staTable->item(iRow,1)->isSelected()) {
      flg[iRow] = true;
    }
    else {
      flg[iRow] = false;
    }
  }
  for (int iRow = nRows-1; iRow >= 0; iRow--) {
    if (flg[iRow]) {
      _staTable->removeRow(iRow);
    }
  }
}

//  PPP Text
////////////////////////////////////////////////////////////////////////////
void t_pppWidgets::slotPPPTextChanged(){

  const static QPalette paletteWhite(QColor(255, 255, 255));
  const static QPalette paletteGray (QColor(230, 230, 230));

  // SNX TRO file sampling
  // ---------------------
  if (sender() == 0 || sender() == _snxtroPath) {
    if ( _snxtroPath->text() != "" ) {
      _snxtroSampl  ->setEnabled(true);
      _snxtroIntr   ->setEnabled(true);
      _snxtroAc     ->setEnabled(true);
      _snxtroSolId  ->setEnabled(true);
      _snxtroSolType->setEnabled(true);
      _snxtroCampId ->setEnabled(true);
      _snxtroSampl  ->setPalette(paletteWhite);
      _snxtroIntr   ->setPalette(paletteWhite);
      _snxtroAc     ->setPalette(paletteWhite);
      _snxtroSolId  ->setPalette(paletteWhite);
      _snxtroSolType->setPalette(paletteWhite);
      _snxtroCampId ->setPalette(paletteWhite);
    }
    else {
    _snxtroSampl  ->setEnabled(false);
    _snxtroIntr   ->setEnabled(false);
    _snxtroAc     ->setEnabled(false);
    _snxtroSolId  ->setEnabled(false);
    _snxtroSolType->setEnabled(false);
    _snxtroCampId ->setEnabled(false);
    _snxtroSampl  ->setPalette(paletteGray);
    _snxtroIntr   ->setPalette(paletteGray);
    _snxtroAc     ->setPalette(paletteGray);
    _snxtroSolId  ->setPalette(paletteGray);
    _snxtroSolType->setPalette(paletteGray);
    _snxtroCampId ->setPalette(paletteGray);
    }
  }


}
