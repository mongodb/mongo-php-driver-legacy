--TEST--
MongoDB::authenticate()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
<?php die("skip requires authenticated enviornment") ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = hostname();
$port = standalone_port();
$dbname = dbname();
$username = "";
$password = "";

$m = new mongo("$host:$port");
$db = $m->selectDB(dbname());
$result = $db->authenticate($username, $password);
echo (int) $result['ok'] . "\n";

$result = $db->authenticate($username, $password.'wrongPass');
echo (int) $result['ok'] . "\n";
?>
--EXPECTF--
%s: Function MongoDB::authenticate() is deprecated in %s on line %d
1

%s: Function MongoDB::authenticate() is deprecated in %s on line %d

Warning: MongoDB::authenticate(): You can't authenticate an already authenticated connection. in %s on line %d
0
