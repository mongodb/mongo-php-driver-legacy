--TEST--
Test for PHP-511: Setting slaveOkay on MongoDB doesn't get passed properly to MongoCollection
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
--FILE--
<?php
$mentions = array(); 
require_once "tests/utils/server.inc";

$m = mongo();
$db = $m->selectDB(dbname());

MongoLog::setModule( MongoLog::RS );
MongoLog::setLevel( MongoLog::FINE );
MongoLog::setCallback( function($a, $b, $message) { if (preg_match('/connection: type: ([A-Z]+),/', $message, $m )) { @$GLOBALS['mentions'][$m[1]]++; }; } );
$db->setSlaveOkay(true);

$mentions = array();

// Normal find
$ret = $db->safe->find(array("doc" => 1));
iterator_to_array($ret);
var_dump($mentions); $mentions = array();
 
// Force primary for command
$db->setProfilingLevel(42);
var_dump($mentions); $mentions = array();
 
// Normal find
$ret = $db->safe->find(array("doc" => 1));
iterator_to_array($ret);
var_dump($mentions); $mentions = array();
?>
--EXPECTF--
Deprecated: Function MongoDB::setSlaveOkay() is deprecated in %sbug00511.php on line %d
array(2) {
  ["PRIMARY"]=>
  int(5)
  ["SECONDARY"]=>
  int(15)
}
array(1) {
  ["PRIMARY"]=>
  int(5)
}
array(2) {
  ["PRIMARY"]=>
  int(5)
  ["SECONDARY"]=>
  int(15)
}
