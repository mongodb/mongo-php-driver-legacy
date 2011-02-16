CC = gcc
PHP_PATH = $(HOME)/php/php-5.3.3/install
INCLUDES = -I./ -I../ -I$(PHP_PATH)/include/php -I$(PHP_PATH)/include/php/main -I$(PHP_PATH)/include/php/TSRM -I$(PHP_PATH)/include/php/Zend 
TEST_OBJS = unit.o mongo.o bson.o db.o collection.o cursor.o gridfs.o mongo_types.o util/hash.o lib/test_mongo.o
LIB_PATH = -L$(PHP_PATH)/lib 
LIBS = -lphp5
BINARY = unit

all: $(TEST_OBJS)
	$(CC) $(LIB_PATH) $(LIBS) -o $(BINARY) $(TEST_OBJS) 

unit.o: unit.c unit.h ../php_mongo.h
	$(CC) -c $(INCLUDES) -o $@ unit.c
lib/test_mongo.o: lib/test_mongo.c lib/test_mongo.h
	$(CC) -c $(INCLUDES) -o $@ lib/test_mongo.c
mongo.o: ../mongo.c ../php_mongo.h ../db.h ../cursor.h ../mongo_types.h ../bson.h ../util/hash.h
	$(CC) -c $(INCLUDES) -o $@ ../mongo.c
bson.o: ../bson.c ../php_mongo.h ../bson.h ../mongo_types.h
	$(CC) -c $(INCLUDES) -o $@ ../bson.c
db.o: ../db.c ../db.h ../php_mongo.h ../collection.h ../cursor.h ../gridfs.h ../mongo_types.h
	$(CC) -c $(INCLUDES) -o $@ ../db.c
collection.o: ../collection.c ../php_mongo.h ../collection.h ../cursor.h ../bson.h ../mongo_types.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../collection.c
cursor.o: ../cursor.c ../cursor.h ../php_mongo.h ../collection.h ../bson.h ../mongo_types.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../cursor.c
gridfs.o: ../gridfs.c ../gridfs.h ../cursor.h ../php_mongo.h ../collection.h ../mongo_types.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../gridfs.c
mongo_types.o: ../mongo_types.c ../mongo_types.h ../php_mongo.h ../collection.h ../bson.h ../db.h
	$(CC) -c $(INCLUDES) -o $@ ../mongo_types.c
util/hash.o: ../util/hash.c ../util/hash.h
	-mkdir $(@D)
	$(CC) -c $(INCLUDES) -o $@ ../util/hash.c

.PHONY: clean

clean:
	-rm -r util
	-rm *.o $(BINARY)
