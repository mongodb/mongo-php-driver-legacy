--TEST--
MongoCursor::info() (32bit, native_long=1, long_as_object=0)
--SKIPIF--
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
ini_set('mongo.native_long', 1);
require_once "tests/utils/server.inc";

$host = MongoShellServer::getShardInfo();
$mc = new MongoClient($host[0]);
$c = $mc->selectCollection(dbname(), 'info');
$c->drop();

for ($i = 0; $i < 105; $i++) {
	$c->insert(array('test' => $i));
}

$r = $c->find();
$foo = $r->next();
try {
	$info = $r->info();
} catch (MongoCursorException $e) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECTF--
int(23)
string(%d) "Can not natively represent the long %d on this platform"
