#!/bin/bash

cd `dirname $0`

pgns=$(find . -type f -name '*.in')
num_tests=0
passed=0
failed=0

for pgn in $pgns; do
	out=$(echo -n $pgn | sed 's/in$/out/')
	if [ -e "$out" ]; then
		echo "Testing $pgn..."
		num_tests=$((tests_run+1))

		./test-pgn "$pgn" | diff - "$out"
		if [ $? -ne 0 ]; then
			echo
			echo Failed on `basename "$pgn"`

			failed=$((failed+1))
		else
			passed=$((passed+1))
		fi
	fi
done

echo

if [ $num_tests -eq $passed ]; then
	echo "All $num_tests tests passed"
else
	echo "$failed / $num_tests tests failed"
	exit 1
fi
