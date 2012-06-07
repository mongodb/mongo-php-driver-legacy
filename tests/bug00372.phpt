--TEST--
PHP-372 (Error codes not being passed to MongoGridFSException)
--FILE--
<?php
/*-----------------------------------------------------------
Test if error code is being passed to MongoGridFSException

Specs:
nginx: 1.1.19
php-fpm: 5.4.0
mongodb: 2.0.4
PHP mongo driver: 1.3.0dev (16th Apr 2012)
-----------------------------------------------------------*/

#Connect to GridFS
$ip = '127.0.0.1';
$port = 27017;
$db = 'test_db';
$prefix = 'test_prefix';
$m = new Mongo('mongodb://'.$ip.':'.$port.'/'.$db);
$GridFS = $m->selectDB($db)->getGridFS($prefix);

#Remove all files from test_db
$GridFS->remove();

#Add unique index on 'filename'
$GridFS->ensureIndex(array('filename'=>1),array('unique'=>true));

#Save first test.txt
try{
    $GridFS->storeBytes('1234567890',array('filename'=>'test.txt'));
}catch (MongoGridFSException $e) {
    echo "error message: ".$e->getMessage()."\n";
    echo "error code: ".$e->getCode()."\n";
}

#Save second test.txt
try{
    $GridFS->storeBytes('1234567890',array('filename'=>'test.txt'));
}catch (MongoGridFSException $e) {
    echo "error message: ".$e->getMessage()."\n";
    echo "error code: ".$e->getCode()."\n";
}


/*
-----------------
Expected result:
-----------------
error message: Could not store file: E11000 duplicate key error index: test_db.test_prefix.files.$filename_1 dup key: { : "test.txt" }
error code: 11000

-----------------
Actual result:
-----------------
error message: Could not store file: E11000 duplicate key error index: test_db.test_prefix.files.$filename_1 dup key: { : "test.txt" }
error code: 0
*/

?>
--EXPECT--
error message: Could not store file: E11000 duplicate key error index: test_db.test_prefix.files.$filename_1  dup key: { : "test.txt" }
error code: 11000

