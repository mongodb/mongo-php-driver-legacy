--TEST--
MongoDB::listCollections() Run against secondary directly
--SKIPIF--
<?php $needs = "2.7.5"; ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicaSetInfo();
$dsn = MongoShellServer::getASecondaryNode();
$m = new MongoClient($dsn, array("readPreference" => MongoClient::RP_SECONDARY));
$d = $m->selectDB(dbname());

MongoLog::setModule( MongoLog::ALL );
MongoLog::setLevel( MongoLog::ALL );
MongoLog::setCallback( function($a, $b, $c) { if (preg_match('/forcing/', $c)) { echo $c, "\n"; } } );

$l = $d->listCollections();
?>
DONE
--EXPECTF--
DONE
