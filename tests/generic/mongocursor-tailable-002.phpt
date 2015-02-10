--TEST--
MongoCursor::tailable() with getNext() and hasNext() (initial query matches nothing)
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
$cursor = $coll->find()->tailable();

$i = 0;

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

    if ($i >= 1) {
        echo "Stopping at i >= 1\n";
        break;
    }
}

?>
===DONE===
--EXPECTF--

Loop iteration: 1
hasNext: false
Found nothing
Cursor is dead; breaking loop
===DONE===
