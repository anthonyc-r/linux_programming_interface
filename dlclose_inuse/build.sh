#! /bin/ksh

clean() {
	if [ -d ./obj ]; then
		rm -r ./obj
	fi
}

build() {
	clean
	mkdir ./obj
	echo " "
	echo "Building libwords without version script (libwords.map)"
	cc -g -c -o ./obj/words.o words.c
	cc -shared -o ./obj/libwords.so.1.0 ./obj/words.o

	echo "readelf --syms --use-dynamic obj/libwords.so.1.0  | grep mystrlen"
	readelf --syms --use-dynamic obj/libwords.so.1.0  | grep mystrlen
	echo " "
	echo "Building libwords with version script (libwords.map)"
	cc -g -c -o ./obj/words.o words.c
	cc -shared -o ./obj/libwords.so.1.0 -Wl,--version-script,libwords.map ./obj/words.o

	echo "readelf --syms --use-dynamic obj/libwords.so.1.0  | grep mystrlen"
	readelf --syms --use-dynamic obj/libwords.so.1.0  | grep mystrlen

	cc -g -c -o ./obj/sort.o sort.c
	cc -shared -o ./obj/libsort.so.1.0 ./obj/sort.o
	cc -g -o ./obj/main main.c
}

run() {
	build
	LD_LIBRARY_PATH=./obj ./obj/main
}


op="-";
if [ $1 ]; then
	op=$1
fi

if [ $op = "build" ]; then
	build
elif [ $op = "clean" ]; then
	clean
elif [ $op = "run" ]; then
	run
else
	echo "./build.sh [clean|build|run]"
fi

exit;