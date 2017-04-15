--TEST--
MongoCursor::dead()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$db = $m->selectDB(dbname());
$c = $db->segfault;

$c->insert(array('test' => 1));
$c->insert(array('test' => 2));

$txlogs = $c->find()->limit(2);

foreach($txlogs as $txlog) {
	echo ($txlogs->dead() ? "Dead" : "Not Dead") . "\n";
}
echo ($txlogs->dead() ? "Dead" : "Not Dead") . "\n";

$info = $txlogs->info();
var_dump($info['id']);
?>
--EXPECTF--
Not Dead
Not Dead
Dead
%s(0)
