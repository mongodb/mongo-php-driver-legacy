--TEST--
MongoDB::getCollectionNames()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb(dbname());

$d->listcol->drop();
$d->listcol->insert(array('_id' => 'test'));

echo "without flag\n";
$collections = $d->getCollectionNames();
foreach( $collections as $col )
{
	if ($col == 'system.indexes') {
		echo $col, "\n";
	}
}

echo "with flag\n";
$collections = $d->getCollectionNames(true);
foreach( $collections as $col )
{
	if ($col == 'system.indexes') {
		echo $col, "\n";
	}
}
?>
--EXPECT--
without flag
with flag
system.indexes
