--TEST--
Test for PHP-1111: Driver can not connection to a slave
--SKIPIF--
<?php require_once 'tests/utils/masterslave.inc' ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$info = MongoShellServer::getMasterSlaveInfo();
$master_client = new MongoClient($info['master'], array('w' => 2));
$master_col = $master_client->selectCollection(dbname(), collname(__FILE__));
$slave_client = new MongoClient($info['slave']);
$slave_col = $slave_client->selectCollection(dbname(), collname(__FILE__));

/* This will fail (because a slave is not a master node) */
try {
	$slave_col->insert( array( 'test' => 42 ) );
} catch (MongoWriteConcernException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}

/* This works */
$master_col->drop();
$master_col->insert( array( 'test' => 42 ) );
var_dump($slave_col->findOne());
?>
===DONE===
--EXPECTF--
100%S
%s:%d: not master
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["test"]=>
  int(42)
}
===DONE===
