--TEST--
Test for PHP-1247: MongoClient should not inherit timeout of persistent connection
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

echo "Testing MongoClient with 500ms socketTimeoutMS, 100ms query:\n";

$m1 = new MongoClient($host, array('socketTimeoutMS' => 500));
$c1 = $m1->selectCollection(dbname(), collname(__FILE__));
$c1->drop();
$c1->insert(array('x' => 1));

try {
    $c1->findOne(array('$where' => 'sleep(100) || true'));
    echo "findOne() did not time out\n";
} catch (MongoCursorTimeoutException $e) {
    printf("findOne() timed out: %s\n", $e->getMessage());
}

echo "\nTesting MongoClient with default socketTimeoutMS, 1000ms query:\n";

$m2 = new MongoClient($host);
$c2 = $m2->selectCollection(dbname(), collname(__FILE__));

try {
    $c2->findOne(array('$where' => 'sleep(1000) || true'));
    echo "findOne() did not time out\n";
} catch (MongoCursorTimeoutException $e) {
    printf("findOne() timed out: %s\n", $e->getMessage());
}

printf("\nConnections used: %d\n", count(MongoClient::getConnections()));

echo "\nTesting MongoClient with 100ms socketTimeoutMS, 500ms query:\n";

$m3 = new MongoClient($host, array('socketTimeoutMS' => 100));
$c3 = $m3->selectCollection(dbname(), collname(__FILE__));

try {
    $c3->findOne(array('$where' => 'sleep(500) || true'));
    echo "findOne() did not time out\n";
} catch (MongoCursorTimeoutException $e) {
    printf("findOne() timed out: %s\n", $e->getMessage());
}

printf("\nConnections used: %d\n", count(MongoClient::getConnections()));

?>
===DONE===
--EXPECTF--
Testing MongoClient with 500ms socketTimeoutMS, 100ms query:
findOne() did not time out

Testing MongoClient with default socketTimeoutMS, 1000ms query:
findOne() did not time out

Connections used: 1

Testing MongoClient with 100ms socketTimeoutMS, 500ms query:
findOne() timed out: %s:%d: Read timed out after reading 0 bytes, waited for 0.100000 seconds

Connections used: 0
===DONE===
