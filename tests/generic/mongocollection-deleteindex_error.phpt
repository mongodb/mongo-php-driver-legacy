--TEST--
MongoCollection::deleteIndex() error with invalid params
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$name = dbname();
$d = $m->$name;
$c = $d->deleteIndex;
$ns = $d->selectCollection('system.indexes');
$c->drop();

$c->ensureIndex( array( 'surname' => 1, 'name' => 1 ) );
var_dump( count(iterator_to_array( $ns->find( array( 'ns' => "$name.deleteIndex" ) ) ) ) );

echo "Wrong argument\n";
$c->deleteIndex( 42 );
var_dump( count(iterator_to_array( $ns->find( array( 'ns' => "$name.deleteIndex" ) ) ) ) );
?>
--EXPECTF--
int(2)
Wrong argument

Warning: MongoCollection::deleteIndex(): The key needs to be either a string or an array in %smongocollection-deleteindex_error.php on line %d
int(2)
