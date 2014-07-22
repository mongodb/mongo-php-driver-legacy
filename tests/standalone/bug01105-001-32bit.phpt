--TEST--
Test for PHP-1105: long_as_object = 0 no effect (32-bit) [1].
--SKIPIF--
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$d = $m->selectDb(dbname());
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$category_name = 'biscuits';
$c->insert(array('ct' => $category_name, 'id' => new MongoInt64("5615")));

ini_set('mongo.native_long', 0);
$result = $d->command(array(
	"findAndModify" => collname(__FILE__),
	"query" => array("ct" => (string)$category_name),
	"update" => array('$inc' => array("id" => 1)),
	"upsert" => true,
	"new" => true
));
var_dump($result['value']['id']);

ini_set('mongo.native_long', 1);
$result = $d->command(array(
	"findAndModify" => collname(__FILE__),
	"query" => array("ct" => (string)$category_name),
	"update" => array('$inc' => array("id" => 1)),
	"upsert" => true,
	"new" => true
));
var_dump($result['value']['id']);

?>
--EXPECTF--
int(5616)

%s error: ini_set(): To prevent data corruption, you are not allowed to turn on the mongo.native_long setting on 32-bit platforms in %s on line %d
