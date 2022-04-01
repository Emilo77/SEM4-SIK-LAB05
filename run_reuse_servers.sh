#!/bin/bash

if [ $# -ne 2 ]
then
  echo "Usage: $0 <port> <number of servers>"
  exit 1
fi

echo "Launching $2 servers:"

for i in $(seq $2)
do
  ./reuse_server $1 &
done

echo "Servers may be running now."