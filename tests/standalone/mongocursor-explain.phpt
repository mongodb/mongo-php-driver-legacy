--TEST--
MongoCursor::explain()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();
$c->insert( array( 'test' => 42 ) );

$result = $c->find( array( 'test' => array( '$gte' => 40 ) ) )->explain();
echo gettype( $result ), "\n";
?>
--EXPECTF--
array
