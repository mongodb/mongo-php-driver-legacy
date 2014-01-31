--TEST--
Test for PHP-963: Requesting Info on Dead MongoCursor Causes Segfault.
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
$c->insert(array('test' => 3));
$c->insert(array('test' => 4));
$c->insert(array('test' => 5));
$c->insert(array('test' => 6));
$c->insert(array('test' => 7));

$txlogs = $c->find()->limit(5);

foreach($txlogs as $txlog) {
	echo ($txlogs->dead() ? "Dead" : "Not Dead") . "\n";
}
echo ($txlogs->dead() ? "Dead" : "Not Dead") . "\n";

$info = $txlogs->info();
var_dump($info['id'], $info['at']);
echo "ALIVE";
?>
--EXPECT--
Not Dead
Not Dead
Not Dead
Not Dead
Not Dead
Dead
int(0)
int(5)
ALIVE
