--TEST--
Mongo::listDBs()
--SKIPIF--
<?php require dirname(__FILE__) . '/skipif.inc'; ?>
--FILE--
<?php
require dirname(__FILE__) ."/../utils.inc";

$m = mongo("admin");
$dbs = $m->listDBs();
var_dump($dbs['ok']);
var_dump(isset($dbs['totalSize']));
var_dump(isset($dbs['databases']) && is_array($dbs['databases']));
?>
--EXPECTF--
float(1)
bool(true)
bool(true)
