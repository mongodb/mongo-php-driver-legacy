--TEST--
MongoCollection::getName()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();

$c = $m->phpunit->col;
echo "Working with collection " . $c->getName() . ".\n";
$c = $m->phpunit->col->foo;
echo "Working with collection " . $c->getName() . ".\n";
$c = $m->phpunit->col->foo->bar;
echo "Working with collection " . $c->getName() . ".\n";
?>
--EXPECT--
Working with collection col.
Working with collection col.foo.
Working with collection col.foo.bar.
