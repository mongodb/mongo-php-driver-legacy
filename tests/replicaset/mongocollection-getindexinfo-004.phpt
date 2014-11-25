--TEST--
MongoCollection::getIndexInfo() Run against secondary directly
--SKIPIF--
<?php $needs = "2.7.5"; ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicaSetInfo();
$dsn = MongoShellServer::getASecondaryNode();
$m = new MongoClient($dsn, array("readPreference" => MongoClient::RP_SECONDARY));
$c = $m->selectCollection(dbname(), collname(__FILE__));

MongoLog::setModule( MongoLog::ALL );
MongoLog::setLevel( MongoLog::ALL );
MongoLog::setCallback( function($a, $b, $c) { if (preg_match('/forcing/', $c)) { echo $c, "\n"; } } );

$l = $c->getIndexInfo();
?>
DONE
--EXPECTF--
DONE
