--TEST--
MongoDB::authenticate()
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
exec('mongo tests/addUser.js');
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB('admin');

$result = $db->authenticate('testUser', 'testPass');
echo (int) $result['ok'] . "\n";

$result = $db->authenticate('testUser', 'wrongPass');
echo (int) $result['ok'] . "\n";
?>
--EXPECT--
1
0
