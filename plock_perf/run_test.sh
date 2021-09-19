#!/bin/bash

TARGET=/tmp/lock_testfile.tmp

stat lockn
if [ ! $? -eq 0 ]; then
	cc -o lockn lockn.c
fi

stat nlock
if [ ! $? -eq 0 ]; then
	cc -o nlock nlock.c
fi

stat $TARGET
if [ $? -eq 0 ]; then

	echo lock_testfile.tmp exists, deleting
	rm $TARGET
fi

dd iflag=count_bytes count=100000 if=/dev/zero of=$TARGET
./nlock $TARGET 40000 &
echo It takes a fairly long time to accuire all 40000 locks...
sleep 30
echo lock 10
time ./lockn $TARGET 10
echo lock 100
time ./lockn $TARGET 100
echo lock 1000
time ./lockn $TARGET 1000
echo lock 10000
time ./lockn $TARGET 10000
echo lock 20000
time ./lockn $TARGET 20000
echo lock 30000
time ./lockn $TARGET 30000
kill $!

rm $TARGET
