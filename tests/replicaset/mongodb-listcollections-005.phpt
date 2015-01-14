--TEST--
MongoDB::listCollections() Run against secondary with RP (legacy mode)
--SKIPIF--
<?php $needs = "2.7.5"; $needsOp = "<"; ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicaSetInfo();
$m = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname'], 'readPreference' => MongoClient::RP_SECONDARY));
$d = $m->selectDB(dbname());

MongoLog::setModule( MongoLog::ALL );
MongoLog::setLevel( MongoLog::ALL );
MongoLog::setCallback( function($a, $b, $c) {
	static $foundPick = 0;
	
	if (preg_match('/finding.*\(read\)/', $c)) {
		$foundPick = 1;
	}
	if ($foundPick == 1 && preg_match('/^pick server/', $c)) {
		$foundPick = 2;
	}
	if ($foundPick == 2 && preg_match('/type:/', $c)) {
		echo $c, "\n";
	}
} );

$l = $d->listCollections();
?>
DONE
--EXPECTF--
- connection: type: SECONDARY, socket: %d, ping: %d, hash: %s:%d;%s;.;%d
DONE
