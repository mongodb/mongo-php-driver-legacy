#!/bin/bash

export CFLAGS="-Wall -Wextra -Wdeclaration-after-statement -Wmissing-field-initializers -Wshadow -Wno-unused-parameter -ggdb3"
phpize && ./configure --with-mongo-sasl > /dev/null && \
make clean && make -j 5 all > /dev/null && make install
