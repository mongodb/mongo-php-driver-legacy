--TEST--
MongoCollection::getName()
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$m = mongo();

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
