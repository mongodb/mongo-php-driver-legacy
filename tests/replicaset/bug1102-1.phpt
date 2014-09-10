--TEST--
Test for PHP-1102: php driver throws exception when connect string contains an unresolvable hostname (1)
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cfg = MongoShellServer::getReplicaSetInfo();
$opts = array('replicaSet' => $cfg['rsname']);

printLogs(MongoLog::ALL, MongoLog::WARNING, "/blacklisted/");

$hosts = $cfg["hosts"][0] . ",invalid-hostname.example.com:27018";
$mc = new MongoClient($hosts, $opts);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

$mc->test->fixtures->findOne();
echo "All good\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Couldn't connect to 'invalid-hostname.example.com:27018': Previous connection attempts failed, server blacklisted
Couldn't connect to 'invalid-hostname.example.com:27018': Previous connection attempts failed, server blacklisted
All good
===DONE===

