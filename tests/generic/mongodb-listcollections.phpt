--TEST--
MongoDB::listCollections()
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
$collections = $d->listCollections();
sort( $collections );
foreach( $collections as $col )
{
	if ($col->getName() == 'system.profile' || $col->getName() == 'listcol') {
		echo $col->getName(), "\n";
	}
}

function sortObject( $a, $b )
{
	return strcmp( $a->getName(), $b->getName() );
}

echo "with flag\n";
$collections = $d->listCollections(true);
usort( $collections, 'sortObject' );
foreach( $collections as $col )
{
	if ($col->getName() == 'system.profile' || $col->getName() == 'listcol') {
		echo $col->getName(), "\n";
	}
}
--EXPECT--
without flag
listcol
with flag
listcol
system.profile
