--TEST--
Test for PHP-776: MongoCollection::batchInsert() with empty options array segfaults
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$documents = array(
    array('_id' => 1),
    array('_id' => 2),
);

$m = mongo_standalone();
$c = $m->selectCollection(dbname(), 'bug00776');
$c->drop();

$c->batchInsert($documents, array());

var_dump($c->count());
?>
--EXPECT--
int(2)
