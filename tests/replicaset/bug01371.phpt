--TEST--
Test for PHP-1371: Memory leak with connection replica set tags
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$rs = MongoShellServer::getReplicasetInfo();

$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
===DONE===
