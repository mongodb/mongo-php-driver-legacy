--TEST--
Connection strings: toString.
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
<?php exit("skip This test doesn't make whole lot of sense"); ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cstring = "$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT,$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT";

$a = new Mongo("mongodb://$cstring");
var_dump($a->__toString());

?>
--EXPECTF--
string(%d) "[%s:%d],%s:%d""
