#!/usr/bin/env bash

{ sleep 2; echo "OFF"; } | ./lab4b | grep ".*:.*:.*" &> /dev/null
if [ $? == 0 ]; then
	echo "Passed smoke test"
else
	echo "Failed smoke test"
fi
