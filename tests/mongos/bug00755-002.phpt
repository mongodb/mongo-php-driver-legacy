--TEST--
Test for PHP-755: Support CursorNotFound query flag (64bit, native)
--SKIPIF--
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
ini_set('mongo.native_long', 1);

$host = MongoShellServer::getShardInfo();
$mc = new MongoClient($host[0]);
$c = $mc->selectCollection(dbname(), 'php755');
$c->drop();

for ($i = 0; $i < 105; $i++) {
	$c->insert(array('test' => $i));
}

$r = $c->find();
$foo = $r->next();
$cur = $r->current();
$info = $r->info();

for ($i = 0; $i < 100; $i++) {
	$foo = $r->next();
	$cur = $r->current();
}

MongoClient::killCursor( $info['server'], $info['id'] );

try {
	for ($i = 0; $i < 105; $i++) {
		$foo = $r->next();
		$cur = $r->current();
	}
} catch (Exception $e) {
	var_dump(get_class($e));
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECTF--
string(20) "MongoCursorException"
int(16336)
string(%d) "%s:%d: could not find cursor %s %s.php755"
