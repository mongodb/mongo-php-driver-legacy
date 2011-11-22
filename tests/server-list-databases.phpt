--TEST--
Server: list databases
--FILE--
<?php
$a = new Mongo("localhost");
$dbs = $a->listDBs();
var_dump($dbs['ok']);
var_dump($dbs['totalSize']);
foreach($dbs['databases'] as $db) {
	if (in_array($db['name'], array('admin', 'local'))) {
		var_dump($db);
	}
}
?>
--EXPECTF--
float(1)
float(%d)
array(3) {
  ["name"]=>
  string(5) "admin"
  ["sizeOnDisk"]=>
  float(%d)
  ["empty"]=>
  bool(false)
}
array(3) {
  ["name"]=>
  string(5) "local"
  ["sizeOnDisk"]=>
  float(%d)
  ["empty"]=>
  bool(true)
}
