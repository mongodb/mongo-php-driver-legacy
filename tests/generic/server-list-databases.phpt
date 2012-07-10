--TEST--
Server: list databases
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ ."/../utils.inc";
$a = mongo("admin");
$dbs = $a->listDBs();
var_dump($dbs['ok']);
var_dump($dbs['totalSize']);
foreach($dbs['databases'] as $db) {
	if (in_array($db['name'], array('local'))) {
		var_dump($db);
	}
}
?>
--EXPECTF--
float(1)
float(%d)
array(3) {
  ["name"]=>
  string(5) "local"
  ["sizeOnDisk"]=>
  float(%d)
  ["empty"]=>
  bool(%s)
}
