--TEST--
MongoCommandCursor::timeout
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

echo "Inserting some data\n";
for ($i = 0; $i < 10 * 1024; $i++) {
	$c->insert( array( '_id' => $i, 'array' => array_range('a','z') ) );
}

echo "Doing grouping\n";

$pipeline = array(
	array( '$unwind' => '$array' )
	array( '$group' => array( "_id" => $
);

$cursor = $c->aggregateCursor( $pipeline );
$cursor->timeout( 1 );

foreach( $cursor as $r) 
{
	var_dump( $r );
}
?>
==DONE==
--EXPECTF--
%s:%d: no such c%s
==DONE==
