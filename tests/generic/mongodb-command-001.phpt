--TEST--
MongoDB::command()
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";

$m = mongo("admin");
$db = $m->selectDb("admin");

$status = $db->command(array('serverStatus' => 1));
var_dump($status['ok']);
?>
--EXPECT--
float(1)
