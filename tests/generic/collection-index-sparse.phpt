--TEST--
Indexes: Sparse
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$m = Mongo();
$db = $m->phpunit;

$db->people->drop();
$db->people->ensureIndex(array('title' => true), array('sparse' => true));
$db->people->insert(array('name' => 'Jim'));
$db->people->insert(array('name' => 'Bones', 'title' => 'Doctor'));

foreach ($db->people->find() as $r) {
	echo @"Name: {$r['name']}; Title: {$r['title']}\n";
}
echo "\n";

foreach ($db->people->find()->sort(array('title' => 1)) as $r) {
	echo @"Name: {$r['name']}; Title: {$r['title']}\n";
}
echo "\n";

$db->people->deleteIndex(array('title' => true));

foreach ($db->people->find()->sort(array('title' => 1)) as $r) {
	echo @"Name: {$r['name']}; Title: {$r['title']}\n";
}
echo "\n";

$db->people->drop();
?>
--EXPECT--
Name: Jim; Title:
Name: Bones; Title: Doctor

Name: Bones; Title: Doctor

Name: Jim; Title:
Name: Bones; Title: Doctor

