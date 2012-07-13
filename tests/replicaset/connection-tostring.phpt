--TEST--
Connection strings: toString.
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
<?php exit("skip This test doesn't make whole lot of sense"); ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";

$cstring = "$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT,$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT";

$a = new Mongo("mongodb://$cstring");
var_dump($a->__toString());

?>
--EXPECTF--
string(%s) "[%s:%d],%s:%d""
