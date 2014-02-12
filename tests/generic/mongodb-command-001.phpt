--TEST--
MongoDB::command()
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--INI--
mongo.long_as_object=1
--FILE--
<?php
require "tests/utils/server.inc";

$m = mongo_standalone("admin");
$db = $m->selectDb("admin");

$status = $db->command(array('serverStatus' => 1));
var_dump((string) $status['ok']);
?>
--EXPECT--
string(1) "1"
