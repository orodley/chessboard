#!/bin/sh

cd `dirname $0`

pgns=$(find . -type f -name '*.pgn')

for pgn in $pgns; do
	./test-pgn $pgn | diff - $pgn
	if [ $? -ne 0 ]; then
		echo
		echo Failed on `basename $pgn`
		exit 1
	fi
done
