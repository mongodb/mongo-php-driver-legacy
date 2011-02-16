CC = gcc
INCLUDES = -I./ -I../ -I~/php/php-5.3.3/install/include/php -I~/php/php-5.3.3/install/include/php/main -I~/php/php-5.3.3/install/include/php/TSRM -I~/php/php-5.3.3/install/include/php/Zend 
TEST_OBJS = unit.o mongo.o bson.o db.o collection.o cursor.o gridfs.o mongo_types.o util/hash.o
LIB_PATH = -L~/php/php-5.3.3/install/lib 
LIBS = -lphp5
BINARY = unit

all: $(TEST_OBJS)
	$(CC) $(LIB_PATH) $(LIBS) -o $(BINARY) $(TEST_OBJS) 

unit.o: unit.c unit.h ../php_mongo.h
	$(CC) -c $(INCLUDES) -o $@ unit.c
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
