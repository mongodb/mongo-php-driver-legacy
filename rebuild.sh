#!/bin/bash

export CFLAGS=-Wall
phpize && ./configure && make clean && make -j 5 all && make install
