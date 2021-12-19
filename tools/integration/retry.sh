#!/usr/bin/env bash

n=0

while true; do
	$@ && break
	if [[ $n -ge 5 ]]; then
		exit -1
	fi
	WAITTIME=$((2**n))
	echo "Execution of $@ failed. Retrying in $WAITTIME seconds..."
	sleep $WAITTIME
	n=$((n+1))
done
