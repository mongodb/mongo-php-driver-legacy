--TEST--
MongoCursor::tailable() with getNext() and hasNext() (eviction of last document kills the cursor)
--SKIPIF--
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
$cursor = $coll->find()->tailable();

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

    try {
        $hasNext = $cursor->hasNext();
    } catch (MongoCursorException $e) {
        printf("Exception: %s\n", $e->getMessage());
        if ($cursor->dead()) {
            echo "Cursor is dead\n";
        }
        echo "Breaking loop\n";
        break;
    }

    printf("hasNext: %s\n", $hasNext ? 'true' : 'false');

    if ($hasNext) {
        $result = $cursor->getNext();
        printf("Found: x=%d\n", $result['x']);
        continue;
    }

    echo "Found nothing\n";

    if ($x >= 5) {
        echo "Stopping at x >= 5\n";
        break;
    }

    /* We cannot delete documents from a capped collection; however, we can
     * cause the last document we iterated to be removed by inserting enough new
     * documents to cause an eviction.
     */
    $doInsert();
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
Inserted: x=3

Loop iteration: 3
Issuing getmore
Exception: %s
Cursor is dead
Breaking loop
===DONE===
