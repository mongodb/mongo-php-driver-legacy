#!/bin/bash
phpize
./configure --quiet
make all install
echo "extension=mongo.so" >> `php --ini | grep "Loaded Configuration" | sed -e "s|.*:\s*||"`

MONGO=`which mongo`
mongod --version
ls $MONGO*
pwd
cp tests/utils/cfg.inc.template tests/utils/cfg.inc

