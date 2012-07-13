--TEST--
MongoDB::authenticate()
--FILE--
<?php
exec('mongo tests/addUser.js');
$mongo = new Mongo('mongodb://localhost');
$db = $mongo->selectDB('admin');

$result = $db->authenticate('testUser', 'testPass');
echo (int) $result['ok'] . "\n";

$result = $db->authenticate('testUser', 'wrongPass');
echo (int) $result['ok'] . "\n";
?>
--EXPECT--
1
0
