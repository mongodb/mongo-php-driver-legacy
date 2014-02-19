--TEST--
MongoResultException due to unrecognized GLE mode
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$c = $mc->selectCollection(dbname(), 'mongocollection-insert_error-001');

try {
    $c->insert(array('x' => 1), array('w' => 'nonsense'));
} catch (MongoException $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
    $document = $e->getDocument();
}
?>
--EXPECTF--
exception class: Mongo%SException
exception message: %s:%d:%S unrecognized getLastError mode: nonsense
exception code: %d
