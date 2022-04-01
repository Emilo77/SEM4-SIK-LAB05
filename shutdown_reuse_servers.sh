#! /bin/bash

if [ $# -ne 1 ]
then
  echo "Usage: $0 <port>"
  exit 1
fi

for i in $(ss -tapn | egrep "[^:]+0\.0\.0\.0:$1" | egrep 'pid=[0-9]+' -o | cut -b 5-)
do
  echo "Shutting down #$i."
  kill $i
done