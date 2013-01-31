--TEST--
Test for PHP-372: Error codes not being passed to MongoGridFSException.
--CREDITS--
Alex Yam
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
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
require_once dirname(__FILE__) ."/../utils.inc";
$db = 'phpunit';
$m = new_mongo($db);
$prefix = 'test_prefix';
$GridFS = $m->selectDB($db)->getGridFS($prefix);

#Remove all files from phpunit
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
?>
--EXPECTF--
error message: Could not store file: %s:%d: E11000 duplicate key error index: phpunit.test_prefix.files.$filename_1  dup key: { : "test.txt" }
error code: 11000

