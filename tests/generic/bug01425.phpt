--TEST--
Test for PHP-1425: explain() does not raise appropriate exception for $err conditions
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$m->admin->command( array( 'setParameter' => 1, 'notablescan' => 1 ) );

$c = $m->selectCollection(dbname(), collname(__FILE__));

$c->drop();
$c->insert( array( 'foo' => 'bar' ) );
try
{
	$c->find( array( 'foo' => 'bar' ) )->explain();
}
catch ( MongoCursorException $e )
{
	echo $e->getCode(), "\n";

	$m->admin->command( array( 'setParameter' => 1, 'notablescan' => 0 ) );
}

$m->admin->command( array( 'setParameter' => 1, 'notablescan' => 0 ) );
?>
--EXPECT--
17007
