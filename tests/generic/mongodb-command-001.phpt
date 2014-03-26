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

echo "Testing with no options argument\n";

$status = $db->command(array('serverStatus' => 1));
var_dump((string) $status['ok']);

echo "\nTesting with empty array options argument\n";

$status = $db->command(array('serverStatus' => 1), array());
var_dump((string) $status['ok']);

echo "\nTesting with null options argument\n";

$status = $db->command(array('serverStatus' => 1), null);
var_dump((string) $status['ok']);
?>
===DONE===
--EXPECT--
Testing with no options argument
string(1) "1"

Testing with empty array options argument
string(1) "1"

Testing with null options argument
string(1) "1"
===DONE===
