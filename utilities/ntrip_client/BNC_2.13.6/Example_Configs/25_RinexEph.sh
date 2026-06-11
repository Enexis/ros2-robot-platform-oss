#!/bin/bash

../bnc -nw -conf /dev/null \
       -key mountPoints "//Example:Configs@products.igs-ip.net:2101/BCEP00BKG0 RTCM_3.3 DEU 50.09 8.66 no 2"\
       -key ephPath Output \
       -key logFile Output/RinexEph.log \
       -key ephIntr "1 hour" \
       -key ephVersion 4 &

psID=`echo $!`
sleep 10
kill $psID

