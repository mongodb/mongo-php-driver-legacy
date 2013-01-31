--TEST--
MongoCollection::findOne() with setSlaveOkay().
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$mongoConnection = new Mongo("mongodb://$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT,$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT", array('replicaSet' => rsname()));
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
