#!/bin/bash

export CFLAGS="-Wall -Wextra -Wdeclaration-after-statement -Wmissing-field-initializers -Wshadow -Wno-unused-parameter -ggdb3"
phpize > /dev/null && \
./configure --with-mongo-sasl > /dev/null && \
make clean && make all > /dev/null && make install
