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

$r = $c->find( array( 'foo' => array( '$near' => array( 5, 5 ) ) ) );

try {
	foreach( $r as $f)
	{
		var_dump($f);
	}
} catch (MongoCursorException $e) {
    echo "got exception\n";
}
?>
--EXPECTF--
got exception
