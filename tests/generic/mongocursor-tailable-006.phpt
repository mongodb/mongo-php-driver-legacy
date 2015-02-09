--TEST--
MongoCursor::tailable() with awaitData(), getNext(), and hasNext()
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_getmore($server, $info) {
    echo "Issuing getmore\n";
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_getmore' => 'log_getmore',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));

$db = $m->selectDB(dbname());
$db->selectCollection(collname(__FILE__))->drop();
$db->createCollection(collname(__FILE__), array('capped' => true, 'size' => 10240, 'max' => 3));

$coll = $db->selectCollection(collname(__FILE__));
$cursor = $coll->find()->tailable()->awaitData();

$x = 0;
$i = 0;

$doInsert = function() use ($coll, &$x) {
    $coll->insert(array('x' => $x));
    printf("Inserted: x=%d\n", $x);
    ++$x;
};

$doInsert();

while (++$i) {
    printf("\nLoop iteration: $i\n", $i);

    $hasNext = $cursor->hasNext();
    printf("hasNext: %s\n", $hasNext ? 'true' : 'false');

    if ($hasNext) {
        $result = $cursor->getNext();
        printf("Found: x=%d\n", $result['x']);
        continue;
    }

    echo "Found nothing\n";

    if ($cursor->dead()) {
        echo "Cursor is dead; breaking loop\n";
        break;
    }

    if ($x >= 5) {
        echo "Stopping at x >= 5\n";
        break;
    }

    $doInsert();
    $doInsert();
}

?>
===DONE===
--EXPECTF--
Inserted: x=0

Loop iteration: 1
hasNext: true
Found: x=0

Loop iteration: 2
Issuing getmore
hasNext: false
Found nothing
Inserted: x=1
Inserted: x=2

Loop iteration: 3
Issuing getmore
hasNext: true
Found: x=1

Loop iteration: 4
hasNext: true
Found: x=2

Loop iteration: 5
Issuing getmore
hasNext: false
Found nothing
Inserted: x=3
Inserted: x=4

Loop iteration: 6
Issuing getmore
hasNext: true
Found: x=3

Loop iteration: 7
hasNext: true
Found: x=4

Loop iteration: 8
Issuing getmore
hasNext: false
Found nothing
Stopping at x >= 5
===DONE===
