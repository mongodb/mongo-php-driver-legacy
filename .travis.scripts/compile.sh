#!/bin/bash
df -h
phpize
./configure --quiet
make all install
echo "extension=mongo.so" >> `php --ini | grep "Loaded Configuration" | sed -e "s|.*:\s*||"`

