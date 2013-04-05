#!/bin/sh

for group in auth-replicaset auth-standalone bridge generic mongos no-servers replicaset replicaset-failover standalone
do
    for extension in diff exp log out mem php sh
    do
        rm -f tests/$group/*.$extension
        rm -f tests/$group/legacy/*.$extension
    done
done
