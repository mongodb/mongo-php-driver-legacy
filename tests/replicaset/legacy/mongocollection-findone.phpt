--TEST--
MongoCollection::findOne() with setSlaveOkay().
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$cfg = MongoShellServer::getReplicasetInfo();

$mongoConnection = new MongoClient($cfg["hosts"][0] . "," . $cfg["hosts"][1], array('replicaSet' => rsname()));
$dbname = dbname();
$db = $mongoConnection->$dbname;
$db->setSlaveOkay();

for ($i = 0; $i < 50; $i++) {
	$user = $db->users->findOne(array('email.address' => 'not-my-email@example.com'));
	var_dump($user);
}
?>
--EXPECTF--
%s: Function MongoDB::setSlaveOkay() is deprecated in %smongocollection-findone.php on line %d
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
NULL
