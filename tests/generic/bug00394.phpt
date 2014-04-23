--TEST--
Test for PHP-394: Crashes and mem leaks.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$x = new MongoClient(array());
var_dump($x);
if ($x) {
    $x->connect();
}
$x = new MongoDB;
var_dump($x);
if ($x) {
    $x->dropCollection(NULL);
}

$x = new MongoDB;
var_dump($x);
if ($x) {
    $x->listCollections();
}

$x = new MongoDB;
var_dump($x);
if ($x) {
    $x->getCollectionNames();
}

$x = new MongoGridFS;
var_dump($x);
if ($x) {
    $x->drop();
}

$x = new MongoGridFSFile;
var_dump($x);
if ($x) {
    $x->getFilename(-1);
}


?>
--EXPECTF--

Warning: MongoClient::__construct() expects parameter 1 to be string, array given in %s on line %d
NULL

Warning: MongoDB::__construct() expects exactly 2 parameters, 0 given in %s on line %d
NULL

Warning: MongoDB::__construct() expects exactly 2 parameters, 0 given in %s on line %d
NULL

Warning: MongoDB::__construct() expects exactly 2 parameters, 0 given in %s on line %d
NULL

Warning: MongoGridFS::__construct() expects at least 1 parameter, 0 given in %s on line %d
NULL

Warning: MongoGridFSFile::__construct() expects at least 2 parameters, 0 given in %s on line %d
NULL


