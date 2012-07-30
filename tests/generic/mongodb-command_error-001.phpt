--TEST--
MongoDB::command() with unsupported database command
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";

$m = mongo();
$db = $m->selectDb('phpunit');
var_dump($db->command(array()));
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
