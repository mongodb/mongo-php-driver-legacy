--TEST--
MongoCursor::dead() with collection size equal to limit
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--INI--
mongo.long_as_object=1
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->insert(array('test' => 1));
$c->insert(array('test' => 2));

$txlogs = $c->find()->limit(2);

foreach($txlogs as $txlog) {
	echo ($txlogs->dead() ? "Dead" : "Not Dead") . "\n";
}
echo ($txlogs->dead() ? "Dead" : "Not Dead") . "\n";

$info = $txlogs->info();
var_dump((string) $info['id']);
?>
--EXPECTF--
Dead
Dead
Dead
%s(1) "0"
