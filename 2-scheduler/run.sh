#!/bin/bash

make

mv libscheduler.so /checker-lin

cd /checker-lin

make -f Makefile.checker

make -f Makefile.checker clean

cd ../
