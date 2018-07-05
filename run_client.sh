#!/bin/bash

if [ "$1" == "" ]; then
    echo "$0 <host>"
    exit -1
fi

build/programs/speedtest_client $1 7 0 22222 11111
