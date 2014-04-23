--TEST--
MongoCollection::find() MongoConnectionException no candidates due to RP tags
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$c = $mc->selectCollection(dbname(), 'mongocollection-find_error-002');
$c->drop();
$c->insert(array('x' => 1), array('w' => 4));

// Use non-matching tags so query has no candidates
try {
    $c->setReadPreference(MongoClient::RP_SECONDARY, array(array('dc' => 'nowhere')));
    $document = $c->findOne(array('x' => 1), array('_id' => 0));
    var_dump($document);
} catch (MongoConnectionException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

// Use matching tags so query can succeed
try {
    $c->setReadPreference(MongoClient::RP_SECONDARY, array(array('dc' => 'ny')));
    $document = $c->findOne(array('x' => 1), array('_id' => 0));
    var_dump($document);
} catch (MongoConnectionException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--
string(26) "No candidate servers found"
%s(71)
array(1) {
  ["x"]=>
  int(1)
}
