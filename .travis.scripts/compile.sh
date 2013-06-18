#!/bin/bash
sudo apt-get install gdb
phpize
./configure --quiet
make all install
echo "extension=mongo.so" >> `php --ini | grep "Loaded Configuration" | sed -e "s|.*:\s*||"`

MONGO=`which mongo`
mongod --version
ls $MONGO*
pwd
cp tests/utils/cfg.inc.template tests/utils/cfg.inc
MONGO_SERVER_STANDALONE=yes MONGO_SERVER_STANDALONE_AUTH=yes MONGO_SERVER_REPLICASET=yes MONGO_SERVER_REPLICASET_AUTH=yes make servers
if [ $? -ne 0 ]; then
    cat /tmp/MONGO-PHP-TESTS*
    cat /tmp/NODE.*
    ps aux | grep mongo
    hostname
    exit 42
fi

