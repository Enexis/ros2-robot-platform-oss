#!/bin/bash

../bnc -nw -conf /dev/null \
       -key reqcAction Edit/Concatenate \
       -key reqcObsFile Input/BRUX00BEL_S_2021125\*_15M_01S_MO.rnx \
       -key reqcRnxVersion 3 \
       -key reqcSampling 30 \
       -key reqcStartDateTime 1967-11-02T00:00:00 \
       -key reqcEndDateTime 2099-01-01T00:00:00 \
       -key reqcNewMarkerName BRUX_MARKER \
       -key reqcOutLogFile Output/RinexConcat.log \
       -key reqcOutObsFile Output/BRUX00BEL_S_20211251100_01H_01S_MO.rnx

