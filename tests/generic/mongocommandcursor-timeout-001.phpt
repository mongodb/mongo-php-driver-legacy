--TEST--
MongoCommandCursor::timeout
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$c = $mc->selectCollection(dbname(), collname(__FILE__));
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

$cursor = $c->aggregateCursor($pipeline, array('cursor' => array('batchSize' => 0)));
$cursor->timeout(1);

try {
    iterator_to_array($cursor, false);
} catch ( MongoCursorTimeoutException $e ) {
    echo $e->getCode(), ': ', $e->getMessage(), "\n";
}

?>
==DONE==
--EXPECTF--
80: %s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
==DONE==
