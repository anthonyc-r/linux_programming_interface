#!/bin/bash

# Test whether it's possible to prevent an exclusive lock form being
# gained by making a series of processes request shared locks.

touch /tmp/my_lock

sflock() {
	echo "$$ Obtaining shared lock..."
	flock -s /tmp/my_lock sleep $1
}

stack_flocks() {
	for i in {0..10}; do
		sflock 2 &
		sleep 1
	done
}

# By overlapping a series of shared locks, an exclusive lock may be
# blocked indefinitely.
stack_flocks &
sleep 1

echo "Attempt to get exclusive lock..."
flock -x /tmp/my_lock echo "Exclusive lock succeeded!"
