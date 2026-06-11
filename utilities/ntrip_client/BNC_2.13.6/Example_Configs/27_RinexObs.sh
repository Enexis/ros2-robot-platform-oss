#!/bin/bash

../bnc -nw -conf /dev/null \
       -key mountPoints "//Example:Configs@euref-ip.net:2101/KIR000SWE0 RTCM_3.3 SWE 67.88 21.06 no 2;//Example:Configs@euref-ip.net:2101/FFMJ00DEU0 RTCM_3.3 DEU 50.09 8.66 no 2" \
       -key logFile Output/RinexObs.log \
       -key rnxPath Output \
       -key rnxIntr "15 min" \
       -key rnxSkel SKL \
       -key rnxSampl "1 sec"\
       -key rnxVersion 4 &

psID=`echo $!`
sleep 10
kill $psID

