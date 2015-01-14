--TEST--
Database: Create collection
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/collection-info.inc";

$a = mongo_standalone();
$d = $a->selectDb(dbname());

// cleanup
$d->dropCollection('create-col1');
var_dump(findCollection($d, 'create-col1'));

// create
$d->createCollection('create-col1');
$retval = findCollection($d, 'create-col1');
var_dump($retval['name']);

?>
--EXPECT--
NULL
string(11) "create-col1"
