--TEST--
MongoCursor::info() (64bit, native_long=1, long_as_object=1)
--SKIPIF--
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
ini_set('mongo.native_long', 1);
ini_set('mongo.long_as_object', 1);

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$c = $mc->selectCollection(dbname(), 'info');
$c->drop();

for ($i = 0; $i < 105; $i++) {
	$c->insert(array('test' => $i));
}

$r = $c->find();
$foo = $r->next();
$info = $r->info();
echo gettype($info['id']), "\n";
echo get_class($info['id']), "\n";
var_dump($info['id']);
?>
--EXPECTF--
object
MongoInt64
object(MongoInt64)#%d (1) {
  ["value"]=>
  string(%d) "%d"
}
