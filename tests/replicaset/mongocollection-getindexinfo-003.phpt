--TEST--
MongoCollection::getIndexInfo() Run against primary
--SKIPIF--
<?php $needs = "2.7.5"; ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicaSetInfo();
$dsn = MongoShellServer::getASecondaryNode();
$m = new MongoClient($dsn, array('replicaSet' => $rs['rsname']));
$c = $m->selectCollection(dbname(), collname(__FILE__));

MongoLog::setModule( MongoLog::ALL );
MongoLog::setLevel( MongoLog::ALL );
MongoLog::setCallback( function($a, $b, $c) { if (preg_match('/forcing/', $c)) { echo $c, "\n"; } } );

$l = $c->getIndexInfo();
?>
DONE
--EXPECTF--
forcing primary for command
DONE
