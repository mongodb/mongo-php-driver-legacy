--TEST--
MongoDB::command() with unsupported database command
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";

$m = mongo_standalone();
$db = $m->selectDb('phpunit');
$retval = $db->command(array());
var_dump($retval["errmsg"], $retval["bad cmd"], $retval["ok"]);
?>
--EXPECTF--
string(%d) "no such c%s: "
array(0) {
}
float(0)
