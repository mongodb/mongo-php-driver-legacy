--TEST--
Test for PHP-1085: w=0 returns unexpected exception on failure (socketTimeoutMS via MongoClient)
--SKIPIF--
<?php $needs = "2.6.0"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php

require_once "tests/utils/server.inc";

function assertFalse($value) {
    if ( ! is_bool($value)) {
        printf("Expected boolean type but received %s\n", gettype($value));
        return;
    }

    if ($value !== false) {
        echo "Expected boolean false but received boolean true\n";
    }
}

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('socketTimeoutMS' => 1));

$db = $mc->selectDB(dbname());
$db->command(array('drop' => collname(__FILE__)), array('socketTimeoutMS' => -1));

$collection = $db->selectCollection(collname(__FILE__));

echo "Testing insert() with w=0\n";

for ($i = 0; $i < 10; ++$i) {
    $retval = $collection->insert(
        array('x' => $i, 'y' => str_repeat('a', 4*1024*1024)),
        array('w' => 0)
    );
    assertFalse($retval);
}

echo "Testing update() with w=0\n";

$retval = $collection->update(
    array('$where' => 'sleep(1) || true'),
    array('$set' => array('y' => 1)),
    array('w' => 0)
);
assertFalse($retval);

echo "Testing remove() with w=0\n";

$retval = $collection->remove(
    array('$where' => 'sleep(1) && false'),
    array('w' => 0)
);
assertFalse($retval);

echo "Testing update() with w=1\n";

try {
    $collection->update(
        array('$where' => 'sleep(1) || true'),
        array('$set' => array('y' => 2)),
        array('w' => 1)
    );
    echo "Expected update() with w=1 to fail but it did not!\n";
} catch (MongoCursorTimeoutException $e) {
    echo "update() with w=1 timed out as expected\n";
}

echo "Testing remove() with w=1\n";

try {
    $collection->remove(
        array('$where' => 'sleep(1) && false'),
        array('w' => 1)
    );
    echo "Expected remove() with w=1 to fail but it did not!\n";
} catch (MongoCursorTimeoutException $e) {
    echo "remove() with w=1 timed out as expected\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
Testing insert() with w=0
Testing update() with w=0
Testing remove() with w=0
Testing update() with w=1
update() with w=1 timed out as expected
Testing remove() with w=1
remove() with w=1 timed out as expected
===DONE===
