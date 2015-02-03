#!/bin/bash
THRESHOLD=300

(iperf -s >/tmp/iperf.log)&
./build/test/picoapp.elf  --vde pic0:/tmp/pic0.ctl:10.50.0.2:255.255.255.0:10.50.0.1: --app iperfc:10.50.0.1: &>/dev/null
killall iperf
SPEED=`cat /tmp/iperf.log |grep Mbits |cut -d " " -f 12`
UNITS=`cat /tmp/iperf.log |grep Mbits |cut -d " " -f 13`

if [ ["$UNITS"] != ["Mbits/sec"] ]; then
    echo "Wrong test result units: expected Mbits/sec, got $UNITS"
    exit 1
fi

if (test $SPEED -lt $THRESHOLD); then 
    echo "Speed too low: expected $THRESHOLD MBits/s, got $SPEED $UNITS"
    exit 2
fi

echo Test result: $SPEED $UNITS

rm -f /tmp/iperf.log
exit 0