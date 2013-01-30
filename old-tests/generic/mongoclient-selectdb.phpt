--TEST--
Mongo::selectDB
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$db = $m->selectDB('test');
echo is_object($db) ? '1' :'0' ;
?>
--EXPECT--
1
