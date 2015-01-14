--TEST--
MongoDB::listCollections() Run against secondary directly (legacy mode)
--SKIPIF--
<?php $needs = "2.7.5"; $needsOp = "<"; ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicaSetInfo();
$dsn = MongoShellServer::getASecondaryNode();
$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());

try {
	$l = $d->listCollections();
} catch(MongoCursorException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
DONE
--EXPECTF--
13435
%s:%d: not master and slaveOk=false
DONE
