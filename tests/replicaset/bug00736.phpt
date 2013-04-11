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

$c = $mc->selectCollection(dbname(), 'bug736');
$c->insert(array('x' => 1));
var_dump($c->findOne() !== null);

$mc = new MongoClient($rs['dsn'], array(
    'replicaSet' => $rs['rsname'],
    'readPreference' => MongoClient::RP_SECONDARY_PREFERRED,
    'readPreferenceTags' => array('foo:bar'),
));

echo "MongoClient constructed without empty tag set fallback\n";

$c = $mc->selectCollection(dbname(), 'bug736');

try {
    $doc = $c->findOne();
} catch (MongoConnectionException $e) {
    printf("error message: %s\n", $e->getMessage());
}

$c->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED, array());
var_dump($c->findOne() !== null);

?>
--EXPECT--
MongoClient constructed with empty tag set fallback
bool(true)
MongoClient constructed without empty tag set fallback
error message: No candidate servers found
bool(true)
