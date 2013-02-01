--TEST--
MongoCollection::__toString()
--DESCRIPTION--
Test implicit and explicit __toString
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$c = $m->phpunit->col;
echo "This is collection $c\n";
echo "This is collection ".$c->__toString()."\n";
$c = $m->selectCollection('phpunit', 'col2');
echo "This is collection $c\n";
echo "This is collection ".$c->__toString()."\n";
?>
--EXPECT--
This is collection phpunit.col
This is collection phpunit.col
This is collection phpunit.col2
This is collection phpunit.col2
