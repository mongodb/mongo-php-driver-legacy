CC = gcc
PHP_PATH = $(HOME)/php/php-5.3.3/install
INCLUDES = -I./ -I../ -I$(PHP_PATH)/include/php -I$(PHP_PATH)/include/php/main -I$(PHP_PATH)/include/php/TSRM -I$(PHP_PATH)/include/php/Zend 
TEST_OBJS = build/unit.o build/mongo.o build/bson.o build/db.o build/collection.o build/cursor.o build/gridfs.o build/mongo_types.o build/util/hash.o build/util/pool.o build/util/connect.o build/util/link.o build/util/rs.o build/lib/test_mongo.o build/lib/test_pool.o
LIB_PATH = -L$(PHP_PATH)/lib 
LIBS = -lphp5
BINARY = unit

all: prereqs $(TEST_OBJS)
	$(CC) $(LIB_PATH) $(LIBS) -o $(BINARY) $(TEST_OBJS) 

prereqs:
	shtool mkdir -p ./build/util ./build/lib


build/unit.o: unit.c unit.h ../php_mongo.h
	$(CC) -c $(INCLUDES) -o $@ unit.c
build/lib/test_mongo.o: lib/test_mongo.c lib/test_mongo.h ../mongo.c ../php_mongo.h
	$(CC) -c $(INCLUDES) -o $@ lib/test_mongo.c
build/lib/test_pool.o: lib/test_pool.c lib/test_pool.h ../util/pool.c ../util/pool.h
	$(CC) -c $(INCLUDES) -o $@ lib/test_pool.c

build/mongo.o: ../mongo.c ../php_mongo.h ../db.h ../cursor.h ../mongo_types.h ../bson.h ../util/hash.h
	$(CC) -c $(INCLUDES) -o $@ ../mongo.c
build/bson.o: ../bson.c ../php_mongo.h ../bson.h ../mongo_types.h
	$(CC) -c $(INCLUDES) -o $@ ../bson.c
build/db.o: ../db.c ../db.h ../php_mongo.h ../collection.h ../cursor.h ../gridfs.h ../mongo_types.h
	$(CC) -c $(INCLUDES) -o $@ ../db.c
build/collection.o: ../collection.c ../php_mongo.h ../collection.h ../cursor.h ../bson.h ../mongo_types.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../collection.c
build/cursor.o: ../cursor.c ../cursor.h ../php_mongo.h ../collection.h ../bson.h ../mongo_types.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../cursor.c
build/gridfs.o: ../gridfs.c ../gridfs.h ../cursor.h ../php_mongo.h ../collection.h ../mongo_types.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../gridfs.c
build/mongo_types.o: ../mongo_types.c ../mongo_types.h ../php_mongo.h ../collection.h ../bson.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../mongo_types.c
build/util/hash.o: ../util/hash.c ../util/hash.h
	$(CC) -c $(INCLUDES) -o $@ ../util/hash.c
build/util/pool.o: ../util/pool.c ../util/pool.h
	$(CC) -c $(INCLUDES) -o $@ ../util/pool.c
build/util/link.o: ../util/link.c ../util/link.h
	$(CC) -c $(INCLUDES) -o $@ ../util/link.c
build/util/connect.o: ../util/connect.c ../util/connect.h
	$(CC) -c $(INCLUDES) -o $@ ../util/connect.c
build/util/rs.o: ../util/rs.c ../util/rs.h
	$(CC) -c $(INCLUDES) -o $@ ../util/rs.c



.PHONY: clean

clean:
	-rm -r util
	-rm *.o $(BINARY)
