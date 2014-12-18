--TEST--
MongoDB::getCollectionNames()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$d = $a->selectDb(dbname());

$d->setProfilingLevel(MongoDB::PROFILING_OFF);
$d->system->profile->drop();
$d->createCollection("system.profile", array('capped' => true, 'size' => 5000));

$d->listcol->drop();
$d->listcol->insert(array('_id' => 'test'));

echo "without flag\n";
$collections = $d->getCollectionNames();
sort( $collections );
foreach( $collections as $col )
{
	if ($col == 'system.profile' || $col == 'listcol') {
		echo $col, "\n";
	}
}

echo "with flag\n";
$collections = $d->getCollectionNames(true);
sort( $collections );
foreach( $collections as $col )
{
	if ($col == 'system.profile' || $col == 'listcol') {
		echo $col, "\n";
	}
}
?>
--EXPECT--
without flag
listcol
with flag
listcol
system.profile
