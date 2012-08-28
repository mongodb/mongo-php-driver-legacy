#!/bin/bash

FLAGS="-Wall -ggdb3 -O0 -I.."
FILES="../bson_helpers.c ../collection.c ../connections.c ../manager.c ../mini_bson.c ../parse.c ../read_preference.c ../str.c ../utils.c ../io.c"

gcc $FLAGS -o sc-test1 simplecon-test.c $FILES
gcc $FLAGS -o rc-test1 replicacon-test.c $FILES
gcc $FLAGS -o rp-test1 rp-test-simple1.c $FILES
gcc $FLAGS -o rp-test2 rp-test-complex1.c $FILES
gcc $FLAGS -o hash-test1 test-hash-split.c $FILES
gcc $FLAGS -o parse-test1 parse-test.c $FILES
gcc $FLAGS -o parse-test2 parse-test2.c $FILES
gcc $FLAGS -o shc-test1 shardcon-test.c $FILES
gcc $FLAGS -o auth-test1 authcon-test.c $FILES
