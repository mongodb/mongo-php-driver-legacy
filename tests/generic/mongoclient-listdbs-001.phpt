--TEST--
Mongo::listDBs()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";

$m = mongo_standalone("admin");
$dbs = $m->listDBs();
var_dump($dbs['ok']);
var_dump(isset($dbs['totalSize']));
var_dump(isset($dbs['databases']) && is_array($dbs['databases']));
?>
--EXPECTF--
float(1)
bool(true)
bool(true)
