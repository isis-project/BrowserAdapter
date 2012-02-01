#!/bin/sh

# A simple test script (usually used for checking for memory leaks) to repeatedly 
# start and stop the browser.

iter=1

while :
do
 pid=`luna-send -n 1 palm://com.palm.applicationManager/launch '{"id":"com.palm.app.browser", "params":"{\"target\":\"http://google.com\"}"}' 2>&1| awk -F"\"" '{print $6}'`

 echo "iter: "$iter", app pid: "$pid

 iter=$(($iter+1))

 sleep 7

 luna-send palm://com.palm.applicationManager/close {\"processId\":\"$pid\"}

done

