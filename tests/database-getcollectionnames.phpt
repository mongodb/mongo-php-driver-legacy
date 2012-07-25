--TEST--
Database: Retrieving collection names
--FILE--
<?php
$a = new Mongo("localhost");
$d = $a->selectDb("phpunit");

$d->drop("listcol");
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
