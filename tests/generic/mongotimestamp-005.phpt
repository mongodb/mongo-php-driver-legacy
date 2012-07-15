--TEST--
MongoTimestamp insertion
--SKIPIF--
<?php require __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection('phpunit', 'mongotimestamp');
$coll->drop();

$ts = new MongoTimestamp();

$coll->insert(array('_id' => 1, 'ts' => $ts));
$result = $coll->findOne(array('_id' => 1));
echo get_class($result['ts']) . "\n";
var_dump($result['ts']->sec === $ts->sec);
var_dump($result['ts']->inc === $ts->inc);
?>
--EXPECT--
MongoTimestamp
bool(true)
bool(true)
