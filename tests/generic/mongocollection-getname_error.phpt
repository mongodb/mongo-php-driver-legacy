--TEST--
MongoCollection::getName()
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$m = mongo();

$c = $m->phpunit->col;
// testing an unneeded parameter
echo "Working with collection " . $c->getName('test') . ".\n";
?>
--EXPECT--
Working with collection col.
