--TEST--
Test for PHP-756: Support QueryFailure query flag
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
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
