--TEST--
Indexes: Sparse
--SKIPIF--
<?php $needs = "2.5.3"; $needsOp = 'lt'; require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = new_mongo_standalone();
$db = $m->phpunit;

$db->people->drop();
$db->people->ensureIndex(array('title' => true), array('sparse' => true));
$db->people->insert(array('name' => 'Jim'));
$db->people->insert(array('name' => 'Bones', 'title' => 'Doctor'));

$indexes = $db->people->getIndexInfo();
// Index 0 is _id
var_dump($indexes[1]["name"], $indexes[1]["sparse"]);

$db->people->drop();
?>
--EXPECT--
string(7) "title_1"
bool(true)
