--TEST--
MongoCommandCursor::timeout [2]
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_query($server, $query, $info) {
    printf("Issuing command: %s\n", key($query));

    if (isset($query['cursor']['batchSize'])) {
        printf("Cursor batch size: %d\n", $query['cursor']['batchSize']);
    }
}

function log_getmore($server, $info) {
    echo "Issuing getmore\n";
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_query' => 'log_query',
        'log_getmore' => 'log_getmore',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));

$ns = dbname() . '.' . collname(__FILE__);

$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->insert(array(
    '_id' => 1,
    'foo' => array_fill(0, 100, array_fill(0, 100, array_fill(0, 100, 1))),
));

$pipeline = array(
    array('$unwind' => '$foo'),
    array('$unwind' => '$foo'),
    array('$unwind' => '$foo'),
    array('$group' => array(
        '_id' => '$_id',
        'foo' => array('$push' => '$foo'),
    )),
);

printLogs(MongoLog::CON, MongoLog::ALL, '/stream timeout/');

$cursor = new MongoCommandCursor( $m, $ns, array( 'aggregate' => collname(__FILE__), 'pipeline' => $pipeline, 'cursor' => array('batchSize' => 0)) );
$cursor->timeout(1);

try {
    iterator_to_array($cursor, false);
} catch ( MongoCursorTimeoutException $e ) {
    echo $e->getCode(), ': ', $e->getMessage(), "\n";
}

?>
==DONE==
--EXPECTF--
Issuing command: drop
Issuing command: aggregate
Cursor batch size: 0
Setting the stream timeout to 0.001000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 0.001000
Now setting stream timeout back to 30.000000
Issuing getmore
Setting the stream timeout to 0.001000
80: %s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
==DONE==
