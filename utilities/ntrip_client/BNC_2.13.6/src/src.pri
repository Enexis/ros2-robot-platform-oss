
unix:DEFINES  += _TTY_POSIX_
win32:DEFINES += _TTY_WIN_

# to solve error C2872: 'byte': ambiguous symbol
# in C++17 type std::byte is introduced
# in Windows SDK byte is defined via typedef unsigned char byte
win32:DEFINES += _HAS_STD_BYTE=0

RESOURCES += bnc.qrc

QT += svg
QT += printsupport
QT += widgets
QT += core

unix:QMAKE_CFLAGS_RELEASE   -= -O2
unix:QMAKE_CXXFLAGS_RELEASE -= -O2

win32:QMAKE_CXXFLAGS += /std:c++17

debug:OBJECTS_DIR=.obj/debug
debug:MOC_DIR=.moc/debug
release:OBJECTS_DIR=.obj/release
release:MOC_DIR=.moc/release

release:DEFINES  += SPLITBLOCK
debug:DEFINES    += SPLITBLOCK
#debug:DEFINES   += BNC_DEBUG_OBS
#debug:DEFINES   += BNC_DEBUG_BCE
#debug:DEFINES   += BNC_DEBUG_PPP
#debug:DEFINES   += BNC_DEBUG_SSR
#debug:DEFINES   += BNC_DEBUG_CMB
#debug:DEFINES   += QT_DISABLE_DEPRECATED_BEFORE=0x050F00

!versionAtLeast(QT_VERSION, 5.15.0):error("Use at least Qt version 5.15.0")

# Include Path
# ------------
INCLUDEPATH += . ../newmat ./RTCM3 ./RTCM3/clock_and_orbit ./RTCM \
               ../qwt ../qwtpolar

# Additional Libraries
# --------------------
unix:LIBS  += -L../newmat -lnewmat -L../qwt -L../qwtpolar -lqwtpolar -lqwt
win32:LIBS += -L../newmat/release -L../qwt/release -L../qwtpolar/release \
              -lnewmat -lqwtpolar -lqwt

HEADERS = bnchelp.html bncgetthread.h    bncwindow.h   bnctabledlg.h  \
          bnccaster.h bncrinex.h bnccore.h bncutils.h   bnchlpdlg.h   \
          bncconst.h bnchtml.h bnctableitem.h bnczerodecoder.h        \
          bncnetquery.h bncnetqueryv1.h bncnetqueryv2.h               \
          bncnetqueryrtp.h bncsettings.h latencychecker.h             \
          bncipport.h bncnetqueryv0.h bncnetqueryudp.h                \
          bncnetqueryudp0.h bncudpport.h bnctime.h                    \
          bncserialport.h bncnetquerys.h bncfigure.h                  \
          bncfigurelate.h bncversion.h                                \
          bncfigureppp.h bncrawfile.h                                 \
          bncmap.h bncantex.h bncephuser.h                            \
          bncoutf.h bncclockrinex.h bncsp3.h bncsinextro.h            \
          bncbiassinex.h                                              \
          bncbytescounter.h bncsslconfig.h reqcdlg.h                  \
          ephemeris.h t_prn.h satObs.h                                \
          upload/bncrtnetdecoder.h upload/bncuploadcaster.h           \
          upload/bncrawuploadcaster.h                                 \
          upload/bncrtnetuploadcaster.h upload/bnccustomtrafo.h       \
          upload/bncephuploadcaster.h qtfilechooser.h                 \
          GPSDecoder.h pppInclude.h pppWidgets.h pppModel.h           \
          pppMain.h pppRun.h pppOptions.h pppCrdFile.h pppThread.h    \
          RTCM/RTCM2.h RTCM/RTCM2Decoder.h                            \
          RTCM/RTCM2_2021.h RTCM/rtcm_utils.h                         \
          RTCM3/RTCM3Decoder.h RTCM3/bits.h RTCM3/gnss.h              \
          RTCM3/RTCM3coDecoder.h RTCM3/ephEncoder.h                   \
          RTCM3/crs.h RTCM3/crsEncoder.h                              \
          RTCM3/clock_and_orbit/clock_orbit.h                         \
          RTCM3/clock_and_orbit/clock_orbit_igs.h                     \
          RTCM3/clock_and_orbit/clock_orbit_rtcm.h                    \
          RTCM3/clock_and_orbit/clock_orbit_rtcm_new.h                \
          rinex/rnxobsfile.h                                          \
          rinex/rnxnavfile.h       rinex/corrfile.h                   \
          rinex/reqcedit.h         rinex/reqcanalyze.h                \
          rinex/graphwin.h         rinex/polarplot.h                  \
          rinex/availplot.h        rinex/eleplot.h                    \
          rinex/dopplot.h          orbComp/sp3Comp.h                  \
          combination/bnccomb.h combination/bncbiassnx.h

HEADERS       += serial/qextserialbase.h serial/qextserialport.h
unix:HEADERS  += serial/posix_qextserialport.h
win32:HEADERS += serial/win_qextserialport.h

SOURCES = bncgetthread.cpp  bncwindow.cpp bnctabledlg.cpp             \
          bnccaster.cpp bncrinex.cpp bnccore.cpp bncutils.cpp         \
          bncconst.cpp bnchtml.cpp bnchlpdlg.cpp bnctableitem.cpp     \
          bnczerodecoder.cpp bncnetqueryv1.cpp bncnetqueryv2.cpp      \
          bncnetqueryrtp.cpp bncsettings.cpp latencychecker.cpp       \
          bncipport.cpp bncnetqueryv0.cpp bncnetqueryudp.cpp          \
          bncnetqueryudp0.cpp bncudpport.cpp                          \
          bncserialport.cpp bncnetquerys.cpp bncfigure.cpp            \
          bncfigurelate.cpp bnctime.cpp                               \
          bncfigureppp.cpp bncrawfile.cpp                             \
          bncmap_svg.cpp bncantex.cpp bncephuser.cpp                  \
          bncoutf.cpp bncclockrinex.cpp bncsp3.cpp bncsinextro.cpp    \
          bncbiassinex.cpp                                            \
          ephemeris.cpp t_prn.cpp satObs.cpp                          \
          bncbytescounter.cpp bncsslconfig.cpp reqcdlg.cpp            \
          upload/bncrtnetdecoder.cpp upload/bncuploadcaster.cpp       \
          upload/bncrawuploadcaster.cpp                               \
          upload/bncrtnetuploadcaster.cpp upload/bnccustomtrafo.cpp   \
          upload/bncephuploadcaster.cpp qtfilechooser.cpp             \
          GPSDecoder.cpp pppWidgets.cpp pppModel.cpp                  \
          pppMain.cpp pppRun.cpp pppOptions.cpp pppCrdFile.cpp        \
          pppThread.cpp                                               \
          RTCM/RTCM2.cpp RTCM/RTCM2Decoder.cpp                        \
          RTCM/RTCM2_2021.cpp RTCM/rtcm_utils.cpp                     \
          RTCM3/RTCM3Decoder.cpp                                      \
          RTCM3/RTCM3coDecoder.cpp RTCM3/ephEncoder.cpp               \
          RTCM3/crsEncoder.cpp                                        \
          RTCM3/clock_and_orbit/clock_orbit.cpp                       \
          RTCM3/clock_and_orbit/clock_orbit_igs.cpp                   \
          RTCM3/clock_and_orbit/clock_orbit_rtcm.cpp                  \
          RTCM3/clock_and_orbit/clock_orbit_rtcm_new.cpp              \
          rinex/rnxobsfile.cpp                                        \
          rinex/rnxnavfile.cpp     rinex/corrfile.cpp                 \
          rinex/reqcedit.cpp       rinex/reqcanalyze.cpp              \
          rinex/graphwin.cpp       rinex/polarplot.cpp                \
          rinex/availplot.cpp      rinex/eleplot.cpp                  \
          rinex/dopplot.cpp        orbComp/sp3Comp.cpp                \
          combination/bnccomb.cpp combination/bncbiassnx.cpp

SOURCES       += serial/qextserialbase.cpp serial/qextserialport.cpp
unix:SOURCES  += serial/posix_qextserialport.cpp
win32:SOURCES += serial/win_qextserialport.cpp

RC_FILE = bnc.rc

QT += network

exists(PPP) {
  INCLUDEPATH += PPP
  DEFINES += USE_PPP
  HEADERS += PPP/pppClient.h    PPP/pppObsPool.h   PPP/pppEphPool.h   \
             PPP/pppStation.h   PPP/pppFilter.h    PPP/pppParlist.h   \
             PPP/pppSatObs.h    PPP/pppRefSat.h    PPP/lambda.h       \
			 PPP/ambres.h
  SOURCES += PPP/pppClient.cpp  PPP/pppObsPool.cpp PPP/pppEphPool.cpp \
             PPP/pppStation.cpp PPP/pppFilter.cpp  PPP/pppParlist.cpp \
             PPP/pppSatObs.cpp                     PPP/lambda.cpp     \
			 PPP/ambres.cpp
}

# Check QtWebEngineWidgets Library Existence
# -----------------------------------------
exists("$$[QT_INSTALL_LIBS]/*WebEngineWidgets*") {
    DEFINES += QT_WEBENGINE
    QT      += webenginewidgets
    message("Configured with QtWebEngine")
}

contains(DEFINES, QT_WEBENGINE) {
  HEADERS     += map/bncmapwin.h
  SOURCES     += map/bncmapwin.cpp
  OTHER_FILES += map/map_osm.html
}
else {
  message("QtWebEngineWidgets Library is not available")
}