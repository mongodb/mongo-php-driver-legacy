--TEST--
MongoDB::authenticate()
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
<?php if (!isauth()) { die("skip Requires authenticated environment"); } ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";

$host = hostname();
$port = port();
$dbname = dbname();
$username = username();
$password = password();

$m = new mongo("$host:$port");
$db = $m->selectDB(dbname());
$result = $db->authenticate($username, $password);
echo (int) $result['ok'] . "\n";

$result = $db->authenticate($username, $password.'wrongPass');
echo (int) $result['ok'] . "\n";
?>
--EXPECT--
1
0
