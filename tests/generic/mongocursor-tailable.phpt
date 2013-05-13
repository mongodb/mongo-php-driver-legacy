--TEST--
MongoCursor::tailable().
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$d = $mongo->selectDb(dbname());
$c = $d->capped;
$c->drop();

$d->createCollection( 'capped', true, 10*1024, 10 );

for ( $i = 0; $i < 20; $i++ )
{
	$c->insert(array('foo' => $i));
}

$cur = $c->find();
$cur->tailable()->awaitData();

$start = microtime( true );
for ( $i = 0; $i < 12; $i++ )
{
	$cur->getNext();
}
echo microtime( true ) - $start > 2 ? "AWAIT DATA WAITED\n" : "NO WAITING\n";
?>
--EXPECTF--
%s: MongoDB::createCollection(): This method now accepts arguments as an options array instead of the three optional arguments for capped, size and max elements in %smongocursor-tailable.php on line %d
AWAIT DATA WAITED
