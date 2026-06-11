#!/bin/bash

../bnc -nw -conf /dev/null \
       -key mountPoints "//Example:Configs@euref-ip.net:2101/KIR000SWE0 RTCM_3.3 SWE 67.88 21.06 no 2" \
       -key miscMount KIR000SWE0 \
       -key miscScanRTCM 2 \
       -key miscIntr "2 sec" \
       -key logFile Output/SCAN.log &

psID=`echo $!`
sleep 20
kill $psID

