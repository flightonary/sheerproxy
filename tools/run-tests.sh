#!/bin/bash

SCRIPT_DIR=$(cd $(dirname $0); pwd)

# compile tests
cd $SCRIPT_DIR/../
make clean
./configure
make test
cd ./tests/

# run tests
echo "###################################"
echo "#            run tests            #"
echo "###################################"
for test in $(find . -type f -name "*.out"); do
    echo "-------- $test -------"
    $test
done
