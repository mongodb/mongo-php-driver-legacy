--TEST--
Test for PHP-756: Support QueryFailure query flag
--SKIPIF--
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getShardInfo();
$mc = new MongoClient($host[0]);
$c = $mc->selectCollection(dbname(), 'php756');
$c->drop();
$c->insert(array('test'=>42));

$r = $c->find( array( 'foo' => array( '$geoNear' => array( 5, 5 ) ) ) );

try {
	foreach( $r as $f)
	{
		var_dump($f);
	}
} catch (MongoCursorException $e) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECTF--
int(13038)
string(%d) "%s:%d: can't find any special indices: 2d (needs index), 2dsphere (needs index),  for: { foo: { $geoNear: [ 5, 5 ] } }"
