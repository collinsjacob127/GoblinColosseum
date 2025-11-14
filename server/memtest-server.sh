#!/usr/bin/bash

make

LOGFILE_DIR='logs/'
LOGFILE_NAME='default.log'

if [ ! -z "$1" ]
  then
    LOGFILE_NAME="$1"
fi

LOGFILE_PATH="$LOGFILE_DIR$LOGFILE_NAME"

echo "Logging to $LOGFILE_PATH"

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file="$LOGFILE_PATH" ./server.x

echo ""

grep -A4 'HEAP SUMMARY' "$LOGFILE_PATH"
grep -A4 'ERROR SUMMARY' "$LOGFILE_PATH"

