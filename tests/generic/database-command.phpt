--TEST--
Database: Running a command
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$a = mongo();
$d = $a->selectDb("phpunit");

// broken, unknown command
$x = $d->command(array());
var_dump($x);

$sp = $d->createCollection("system.profile", true, 5000);
$d->command(array('profile' => 1));
$x = $d->command(array('profile' => 0));
var_dump($x);
?>
--EXPECT--
array(3) {
  ["errmsg"]=>
  string(13) "no such cmd: "
  ["bad cmd"]=>
  array(0) {
  }
  ["ok"]=>
  float(0)
}
array(3) {
  ["was"]=>
  int(1)
  ["slowms"]=>
  int(100)
  ["ok"]=>
  float(1)
}
