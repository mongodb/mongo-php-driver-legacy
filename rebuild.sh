#!/bin/bash

phpize && ./configure && make clean && make -j 5 all && make install
