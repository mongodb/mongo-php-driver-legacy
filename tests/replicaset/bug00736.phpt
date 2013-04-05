--TEST--
Test for PHP-736: MongoClient construction with non-matching RP tag sets
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array(
    'replicaSet' => $rs['rsname'],
    'readPreference' => MongoClient::RP_SECONDARY_PREFERRED,
    'readPreferenceTags' => array('foo:bar', ''),
));

echo "MongoClient constructed with empty tag set fallback\n";

$c = $mc->selectCollection(dbname(), 'fixtures');
var_dump($c->findOne());

$mc = new MongoClient($rs['dsn'], array(
    'replicaSet' => $rs['rsname'],
    'readPreference' => MongoClient::RP_SECONDARY_PREFERRED,
    'readPreferenceTags' => array('foo:bar'),
));

echo "MongoClient constructed without empty tag set fallback\n";

$c = $mc->selectCollection(dbname(), 'fixtures');

try {
    echo "Finding one (should fail, we don't have that tag)\n";
    var_dump($c->findOne());
} catch (MongoConnectionException $e) {
    printf("error message: %s\n", $e->getMessage());
}

echo "Secondary read, killing that tag\n";
$c->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED, array());
var_dump($c->findOne());

?>
--EXPECTF--
MongoClient constructed with empty tag set fallback
array(2) {
  ["_id"]=>
  object(MongoId)#6 (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["example"]=>
  string(8) "document"
}
MongoClient constructed without empty tag set fallback
Finding one (should fail, we don't have that tag)
error message: No candidate servers found
Secondary read, killing that tag
array(2) {
  ["_id"]=>
  object(MongoId)#7 (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["example"]=>
  string(8) "document"
}
