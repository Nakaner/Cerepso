#! /usr/bin/env bash
while true
do echo "==============" #> memory-usage.log
	for i in `pgrep postgres`
	do
		echo -n "$i " #> memory-usage.log
		grep VmRSS /proc/$i/status | egrep -o "[0-9]*" #> memory-usage.log
	done
	echo -n $(pgrep pgimporter) #> memory-usage.log
	echo -n " " #> memory-usage.log
	grep VmRSS /proc/$(pgrep pgimporter)/status | egrep -o "[0-9]*" #> memory-usage.log
	sleep 1
done
