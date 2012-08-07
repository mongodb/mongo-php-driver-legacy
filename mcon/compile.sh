#!/bin/bash

#gcc -ggdb3 -O0 -o test2 bson_helpers.c connections.c manager.c mini_bson.c parse.c simplecon-test.c str.c utils.c io.c
#gcc -Wall -ggdb3 -O0 -o test3 bson_helpers.c collection.c connections.c manager.c mini_bson.c parse.c read_preference.c replicacon-test.c str.c utils.c io.c
#gcc -Wall -ggdb3 -O0 -o rp-test1 rp-test-simple1.c bson_helpers.c collection.c connections.c manager.c mini_bson.c parse.c read_preference.c str.c utils.c io.c
gcc -Wall -ggdb3 -O0 -o hash-test1 test-hash-split.c bson_helpers.c collection.c connections.c manager.c mini_bson.c parse.c read_preference.c str.c utils.c io.c
