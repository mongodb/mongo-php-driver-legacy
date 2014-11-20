--TEST--
Test for PHP-953: MongoCollection::ensureIndex() should check full namespace length.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$c = $m->selectCollection(dbname(), collname(__FILE__));

$prefix_len = 3 + strlen(dbname()) + strlen(collname(__FILE__));

$indexSpec1 = array( 'a' => 1 );
$name1 = str_repeat('a', 127 - $prefix_len);
$ok = $c->ensureIndex( $indexSpec1, array( 'name' => $name1 ) );
var_dump( (bool) $ok['ok'] );

$indexSpec2 = array( 'a' => 1 );
$name2 = str_repeat('a', 127 - $prefix_len + 1);

try {
	$c->ensureIndex( $indexSpec2, array( 'name' => $name2 ) );
} catch (MongoException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}

$key1 = str_repeat('a', 125 - $prefix_len);
$indexSpec3 = array( $key1 => 1 );
$ok = $c->ensureIndex( $indexSpec3 );
var_dump( (bool) $ok['ok'] );

$key2 = str_repeat('a', 125 - $prefix_len + 1);
$indexSpec4 = array( $key2 => 1 );
try {
	$c->ensureIndex( $indexSpec4 );
} catch (MongoException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
bool(true)
14
index name (including namespace) too long: 1%d, max 127 characters
bool(true)
14
index name (including namespace) too long: 1%d, max 127 characters
