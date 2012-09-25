--TEST--
MongoCursor::tailable().
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$d = $mongo->selectDb(dbname());
$c = $d->capped;
$d->drop();

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
--EXPECT--
AWAIT DATA WAITED
