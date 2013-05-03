--TEST--
MongoResultException due to unrecognized GLE mode
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

MongoLog::setModule(MongoLog::IO);
MongoLog::setLevel(MongoLog::ALL);
function foo($c, $m) { echo $m, "\n"; } set_error_handler('foo');

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
    var_dump((boolean) $document['ok']);
}
?>
--EXPECTF--
IO      FINE: is_gle_op: yes
IO      FINE: append_getlasterror
IO      FINE: append_getlasterror: added w='nonsense'
IO      FINE: append_getlasterror: added wtimeout=10000 (from collection property)
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
exception class: MongoResultException
exception message: %s:%d: exception: unrecognized getLastError mode: nonsense
exception code: 14830
bool(false)
