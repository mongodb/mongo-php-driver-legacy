--TEST--
MongoBinData insertion with default type
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
error_reporting(-1);

$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'mongobindata');
$coll->drop();

$coll->insert(array('_id' => 1, 'bin' => new MongoBinData('abcdefg')));

$result = $coll->findOne(array('_id' => 1));

echo get_class($result['bin']) . "\n";
var_dump($result['bin']->bin);
var_dump($result['bin']->type);
?>
--EXPECTF--
MongoBinData
string(7) "abcdefg"
int(0)
