--TEST--
Custom "mongo.cmd" INI option with namespaces
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB('admin');

ini_set('mongo.cmd', '@');

$cmd = $db->selectCollection('@cmd');

$info = $cmd->findOne(array('buildinfo' => 1));
var_dump(array_key_exists('version', $info));
var_dump(array_key_exists('gitVersion', $info));
var_dump(array_key_exists('sysInfo', $info));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
