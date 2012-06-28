--TEST--
MongoCollection::getName()
--FILE--
<?php
$m = new Mongo();
$c = $m->phpunit->col;
// testing an unneeded parameter
echo "Working with collection " . $c->getName('test') . ".\n";
?>
--EXPECT--
Working with collection col.
