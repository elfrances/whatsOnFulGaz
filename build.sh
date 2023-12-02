#!/bin/bash

# Simple wrapper to figure out the OS we are building
# on, and pass the info to make via the variable
# 'OS_TYPE_VAL', using the numeric values of the OsTyp
# enum type defined in args.h.

TGT_OS=`uname -o`
OS_TYPE=0

if [ $TGT_OS == Darwin ]
then
    OS_TYPE=1
elif [ $TGT_OS == Cygwin ]
then
    OS_TYPE=2
else
    OS_TYPE=3
fi

make OS_TYPE_VAL=$OS_TYPE all

