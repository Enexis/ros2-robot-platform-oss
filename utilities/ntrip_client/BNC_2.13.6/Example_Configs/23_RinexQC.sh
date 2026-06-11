#!/bin/bash

../bnc -nw -conf /dev/null -display :1 --platform offscreen\
       -key reqcAction Analyze \
       -key reqcObsFile Input/BRUX00BEL_R_20250750000_01D_30S_MO.rnx \
       -key reqcNavFile Input/BRUX00BEL_R_20250750000_01D_MN.rnx \
       -key reqcSkyPlotSignals "G:1&2&5 R:1&2&3 E:1&5&6&7&8 C:1&2&5&6&7" \
       -key reqcOutLogFile Output/RinexQc.log \
       -key reqcPlotDir Output 2>/dev/null


