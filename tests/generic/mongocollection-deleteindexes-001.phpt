--TEST--
MongoCollection::deleteIndexes()
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

echo "Delete all (except the _id index)\n";
$c->ensureIndex( array( 'surname' => 1, 'name' => 1 ) );
var_dump( count(iterator_to_array( $ns->find( array( 'ns' => "$name.deleteIndex" ) ) ) ) );
$c->deleteIndexes();
var_dump( count(iterator_to_array( $ns->find( array( 'ns' => "$name.deleteIndex" ) ) ) ) );

echo "Delete two\n";
$c->ensureIndex( array( 'name' => 1 ) );
$c->ensureIndex( array( 'surname' => 1, 'name' => 1 ) );
var_dump( count(iterator_to_array( $ns->find( array( 'ns' => "$name.deleteIndex" ) ) ) ) );
$c->deleteIndexes();
var_dump( count(iterator_to_array( $ns->find( array( 'ns' => "$name.deleteIndex" ) ) ) ) );
?>
--EXPECT--
Delete all (except the _id index)
int(2)
int(1)
Delete two
int(3)
int(1)
