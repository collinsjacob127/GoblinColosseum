#!/usr/bin/bash 

cd $(dirname $0)

#export GOBLIN_ROOT_CWD="$(pwd)/build/"
#export GOBLIN_ROOT_CWD="TESTING TESTING TESTING"

#echo $GOBLIN_ROOT_CWD

cmake --build build --parallel
