--TEST--
MongoDB::authenticate()
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
<?php if (!isauth()) { die("skip Requires authenticated environment"); } ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

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
--EXPECTF--
%s: Function MongoDB::authenticate() is deprecated in %s on line %d
1

%s: Function MongoDB::authenticate() is deprecated in %s on line %d
0
