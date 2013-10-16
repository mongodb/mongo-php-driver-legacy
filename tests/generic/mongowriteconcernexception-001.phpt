--TEST--
MongoWriteConcernException due to inserting duplicate keys
--SKIPIF--
<?php require_once 'tests/utils/standalone.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$c = $mc->selectCollection(dbname(), 'mongowriteconcernexception-001');
$c->drop();

try {
    $c->insert(array('_id' => 1));
    echo "first document inserted\n";
    $c->insert(array('_id' => 1));
    echo "second document inserted\n";
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
    $document = $e->getDocument();
    var_dump((boolean) $document['ok']);
}
?>
--EXPECTF--
first document inserted
exception class: MongoWriteConcernException
exception message: %s:%d: E11000 duplicate key error %s
exception code: 11000
bool(true)
