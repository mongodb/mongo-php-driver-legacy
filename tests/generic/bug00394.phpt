--TEST--
Test for bug PHP-394: Crashes and mem leaks
--SKIPIF--
<?php require __DIR__ . "/skipif.inc" ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";

$x = new Mongo(array());
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

Warning: Mongo::__construct() expects parameter 1 to be string, array given in %s on line %d
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


