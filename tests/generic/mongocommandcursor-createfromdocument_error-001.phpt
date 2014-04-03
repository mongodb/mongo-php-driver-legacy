--TEST--
MongoCommandCursor::createFromDocument() with nonsense arguments
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$dbname = dbname();

$m = new MongoClient($dsn);

$connections = $m->getConnections();
$hash = $connections[0]['hash'];

$c = MongoCommandCursor::createFromDocument(
	$m,
	$hash,
	array(
		"cursor" => array(
			"id" => new MongoInt64(1), 
			"ns" => "test.cursor", 
			"firstBatch" => array(array("foo" => "bar"))
		)
	)
);

try {
	foreach ($c as $k => $v) {
	} 
} catch (MongoCursorException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
17356
%s:%d: collection dropped between getMore calls
