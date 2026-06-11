#ifndef RTCM3_CLOCK_ORBIT_RTCM_NEW_H
#define RTCM3_CLOCK_ORBIT_RTCM_NEW_H

/* Programheader

 Name:           clock_orbit_rtcm_new.h
 Project:        RTCM3
 Version:        $Id: clock_orbit_rtcm_new.h 9050 2020-08-27 14:01:42Z stuerze $
 Authors:        Dirk Stöcker, Andrea Stürze
 Description:    state space approach: RTCM_NEW
 */

#include <string.h>
#include "clock_orbit.h"

class SsrCorrRtcmNew: public SsrCorr {

public:
  SsrCorrRtcmNew();
  ~SsrCorrRtcmNew();

  void setCorBase() {
    COBBASE_GPS     = 1057;
    COBBASE_GLONASS =   41;
    COBBASE_GALILEO =   62;
    COBBASE_BDS     =   63;
    COBBASE_QZSS    =   64;
    COBBASE_NUM     =    5;
    corbase
        << COBBASE_GPS
        << COBBASE_GLONASS
        << COBBASE_GALILEO
        << COBBASE_BDS
        << COBBASE_QZSS;
  };

  void setCorOffset() { };

  void setCoType() {
    COTYPE_GPSORBIT        = COBBASE_GPS;
    COTYPE_GPSCLOCK        = COTYPE_GPSORBIT + 1;
    COTYPE_GPSCOMBINED     = COTYPE_GPSCLOCK + 2;
    COTYPE_GPSURA          = COTYPE_GPSCOMBINED + 1;
    COTYPE_GPSHR           = COTYPE_GPSURA + 1;

    COTYPE_GLONASSORBIT    = COBBASE_GLONASS;
    COTYPE_GLONASSCLOCK    = COTYPE_GLONASSORBIT + 1;
    COTYPE_GLONASSCOMBINED = COTYPE_GLONASSCLOCK + 2;
    COTYPE_GLONASSURA      = COTYPE_GLONASSCOMBINED + 1;
    COTYPE_GLONASSHR       = COTYPE_GLONASSURA + 1;

    COTYPE_GALILEOORBIT    = COBBASE_GALILEO;
    COTYPE_GALILEOCLOCK    = COTYPE_GALILEOORBIT + 3;
    COTYPE_GALILEOCOMBINED = COTYPE_GALILEOCLOCK + 6;
    COTYPE_GALILEOURA      = COTYPE_GALILEOCOMBINED + 3;
    COTYPE_GALILEOHR       = COTYPE_GALILEOURA + 3;

    COTYPE_BDSORBIT        = COBBASE_BDS;
    COTYPE_BDSCLOCK        = COTYPE_BDSORBIT + 3;
    COTYPE_BDSCOMBINED     = COTYPE_BDSCLOCK + 6;
    COTYPE_BDSURA          = COTYPE_BDSCOMBINED + 3;
    COTYPE_BDSHR           = COTYPE_BDSURA + 3;

    COTYPE_QZSSORBIT       = COBBASE_QZSS;
    COTYPE_QZSSCLOCK       = COTYPE_QZSSORBIT + 3;
    COTYPE_QZSSCOMBINED    = COTYPE_QZSSCLOCK + 6;
    COTYPE_QZSSURA         = COTYPE_QZSSCOMBINED + 3;
    COTYPE_QZSSHR          = COTYPE_QZSSURA + 3;

    COTYPE_AUTO = 0;
  };

  void setCbType() {
    CBTYPE_GPS     = COBBASE_GPS     + 2;
    CBTYPE_GLONASS = COBBASE_GLONASS + 2;
    CBTYPE_GALILEO = COBBASE_GALILEO + 6;
    CBTYPE_QZSS    = COBBASE_QZSS    + 6;
    CBTYPE_BDS     = COBBASE_BDS     + 6;
    CBTYPE_AUTO = 0;
  };

  void setPbType() {
    PBTYPE_BASE    = 85;
    PBTYPE_GPS     = PBTYPE_BASE;
    PBTYPE_GLONASS = PBTYPE_GPS++;
    PBTYPE_GALILEO = PBTYPE_GLONASS++;
    PBTYPE_BDS     = PBTYPE_GALILEO++;
    PBTYPE_QZSS    = PBTYPE_BDS++;
    PBTYPE_AUTO = 0;
  };

  void setPbExtType() {
    PBEXTTYPE_BASE    = 90;
    PBEXTTYPE_GPS     = PBEXTTYPE_BASE;
    PBEXTTYPE_GLONASS = PBEXTTYPE_GPS++;
    PBEXTTYPE_GALILEO = PBEXTTYPE_GLONASS++;
    PBEXTTYPE_BDS     = PBEXTTYPE_GALILEO++;
    PBEXTTYPE_QZSS    = PBEXTTYPE_BDS++;
    PBEXTTYPE_AUTO = 0;
  };

  void setVtecType() {};

  void setSatAntType() {
    SATANTTYPE_BASE    = 80;
    SATANTTYPE_GPS     = SATANTTYPE_BASE;
    SATANTTYPE_GLONASS = SATANTTYPE_GPS++;
    SATANTTYPE_GALILEO = SATANTTYPE_GLONASS++;
    SATANTTYPE_BDS     = SATANTTYPE_GALILEO++;
    SATANTTYPE_QZSS    = SATANTTYPE_BDS++;
    SATANTTYPE_AUTO    = 0;
  };

  void setGridDefType() {
    GRID_BASE = 61;
  }

  void setTropoType() {
    TROPOTYPE_BASE = 95;
  }

  void setRegIonoType() {
    REGIONOTYPE_BASE    = 96;
    REGIONOTYPE_GPS     = REGIONOTYPE_BASE;
    REGIONOTYPE_GLONASS = REGIONOTYPE_GPS++;
    REGIONOTYPE_GALILEO = REGIONOTYPE_GLONASS++;
    REGIONOTYPE_BDS     = REGIONOTYPE_GALILEO++;
    REGIONOTYPE_QZSS    = REGIONOTYPE_BDS++;
    REGIONOTYPE_AUTO    = 0;
  }

  void setMetaDataType() {
    METADATATYPE_BASE = 60;
  }

  void setCodeType() {
    CODETYPE_RESERVED = 99;
    // GPS
    CODETYPE_GPS_L1_CA          =  0;
    CODETYPE_GPS_L1_P           =  1;
    CODETYPE_GPS_L1_Z           =  2;
    // 3 + 4 RESERVED
    CODETYPE_GPS_L2_CA          =  5;
    CODETYPE_GPS_SEMI_CODELESS  =  6;
    CODETYPE_GPS_L2C_M          =  7;
    CODETYPE_GPS_L2C_L          =  8;
    CODETYPE_GPS_L2C_ML         =  9;
    CODETYPE_GPS_L2_P           = 10;
    CODETYPE_GPS_L2_Z           = 11;
    //12 + 13 RESEVED
    CODETYPE_GPS_L5_I           = 14;
    CODETYPE_GPS_L5_Q           = 15;
    CODETYPE_GPS_L5_IQ          = 16;
    CODETYPE_GPS_L1C_D          = 17;
    CODETYPE_GPS_L1C_P          = 18;
    CODETYPE_GPS_L1C_DP         = 19;
    // > 19 RESEVED

    // GLONASS
    CODETYPE_GLONASS_L1_CA      =  0;
    CODETYPE_GLONASS_L1_P       =  1;
    CODETYPE_GLONASS_L2_CA      =  2;
    CODETYPE_GLONASS_L2_P       =  3;
    // > 3 RESERVED

    // Galileo
    CODETYPE_GALILEO_E1_A       =  0;
    CODETYPE_GALILEO_E1_B       =  1;
    CODETYPE_GALILEO_E1_C       =  2;
    CODETYPE_GALILEO_E1_BC      =  3;
    CODETYPE_GALILEO_E1_ABC     =  4;
    CODETYPE_GALILEO_E5A_I      =  5;
    CODETYPE_GALILEO_E5A_Q      =  6;
    CODETYPE_GALILEO_E5A_IQ     =  7;
    CODETYPE_GALILEO_E5B_I      =  8;
    CODETYPE_GALILEO_E5B_Q      =  9;
    CODETYPE_GALILEO_E5B_IQ     = 10;
    CODETYPE_GALILEO_E5_I       = 11;
    CODETYPE_GALILEO_E5_Q       = 12;
    CODETYPE_GALILEO_E5_IQ      = 13;
    CODETYPE_GALILEO_E6_A       = 14;
    CODETYPE_GALILEO_E6_B       = 15;
    CODETYPE_GALILEO_E6_C       = 16;
    CODETYPE_GALILEO_E6_BC      = 17;
    CODETYPE_GALILEO_E6_ABC     = 18;
    // > 18 RESERVED

    CODETYPE_QZSS_L1_CA         =  0;
    CODETYPE_QZSS_L1C_D         =  1;
    CODETYPE_QZSS_L1C_P         =  2;
    CODETYPE_QZSS_L2C_M         =  3;
    CODETYPE_QZSS_L2C_L         =  4;
    CODETYPE_QZSS_L2C_ML        =  5;
    CODETYPE_QZSS_L5_I          =  6;
    CODETYPE_QZSS_L5_Q          =  7;
    CODETYPE_QZSS_L5_IQ         =  8;
    CODETYPE_QZSS_L6_D          =  9;
    CODETYPE_QZSS_L6_P          = 10;
    CODETYPE_QZSS_L6_DP         = 11;
    CODETYPE_QZSS_L1C_DP        = 12;
    // > 12 RESERVED

    // BDS
    CODETYPE_BDS_B1_I           =  0;
    CODETYPE_BDS_B1_Q           =  1;
    CODETYPE_BDS_B1_IQ          =  2;
    CODETYPE_BDS_B3_I           =  3;
    CODETYPE_BDS_B3_Q           =  4;
    CODETYPE_BDS_B3_IQ          =  5;
    CODETYPE_BDS_B2_I           =  6;
    CODETYPE_BDS_B2_Q           =  7;
    CODETYPE_BDS_B2_IQ          =  8;
    CODETYPE_BDS_B1C_D          =  9;
    CODETYPE_BDS_B1C_P          = 10;
    CODETYPE_BDS_B1C_DP         = 11;
    CODETYPE_BDS_B2a_D          = 12;
    CODETYPE_BDS_B2a_P          = 13;
    CODETYPE_BDS_B2a_DP         = 14;
    CODETYPE_BDS_B2b_D          = 15;
    // > 15 RESEVED
  }

  std::string       codeTypeToRnxType(char system, CodeType type);
  SsrCorr::CodeType rnxTypeToCodeType(char system, std::string type);

  size_t MakeClockOrbit(const struct ClockOrbit *co, ClockOrbitType type,
      int moremessagesfollow, char *buffer, size_t size);
  size_t MakeCodeBias(const struct CodeBias *b, CodeBiasType type,
      int moremessagesfollow, char *buffer, size_t size);
  size_t MakePhaseBias(const struct PhaseBias *b, PhaseBiasType type,
      int moremessagesfollow, char *buffer, size_t size);
  size_t MakeVTEC(const struct VTEC *v, int moremessagesfollow, char *buffer,
      size_t size);
  enum GCOB_RETURN GetSSR(struct ClockOrbit *co, struct CodeBias *b,
      struct VTEC *v, struct PhaseBias *pb, const char *buffer, size_t size,
      int *bytesused);
 };

#endif /* RTCM3_CLOCK_ORBIT_RTCM_H */
