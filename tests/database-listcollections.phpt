--TEST--
Database: Enumerating collections
--FILE--
<?php
$a = new Mongo("localhost");
$d = $a->selectDb("phpunit");

$d->drop("listcol");
$d->listcol->insert(array('_id' => 'test'));

echo "without flag\n";
$collections = $d->listCollections();
foreach( $collections as $col )
{
	if ($col->getName() == 'system.indexes') {
		echo $col->getName(), "\n";
	}
}

echo "with flag\n";
$collections = $d->listCollections(true);
foreach( $collections as $col )
{
	if ($col->getName() == 'system.indexes') {
		echo $col->getName(), "\n";
	}
}
--EXPECT--
without flag
with flag
system.indexes
