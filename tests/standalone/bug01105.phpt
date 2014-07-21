--TEST--
Test for PHP-1105: long_as_object = 0 no effect.
--SKIPIF--
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

ini_set('mongo.native_long', 1);

$c->insert(array('ct' => $category_name, 'id' => 5615));
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
