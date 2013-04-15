--TEST--
Test for PHP-759: Write operations apply ReadPreferenceTags when finding primary
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array(
    'replicaSet' => $rs['rsname'],
    'readPreference' => MongoClient::RP_PRIMARY_PREFERRED,
    'readPreferenceTags' => array('dc:ny'),
));

// Doesn't match anything
$mc->setReadPreference(MongoClient::RP_SECONDARY, array(array("foo" => "bar")));

$c = $mc->selectDb(dbname())->fixtures;
$c->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED, array());
var_dump($c->findOne(array("example" => "document")));

try {
    $mc->selectDB(dbname())->random->drop();
    echo "Drop succeeded as it should\n";
} catch(Exception $e) {
    echo "FAILED\n";
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}

?>
--EXPECTF--
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["example"]=>
  string(8) "document"
}
Drop succeeded as it should

