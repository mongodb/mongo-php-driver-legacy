--TEST--
Test for PHP-1065: Mongo driver is crashing during getmore
--SKIPIF--
<?php $needs = "2.6.0"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php

function log_getmore($server, $cursor_options) {
    printf("%s\n", __METHOD__);
}

$ctx = stream_context_create(array(
    "mongodb" => array("log_getmore" => "log_getmore")
));

require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host . "/test?socketTimeoutMS=1", array(), array("context" => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

foreach(range(0, 1000) as $i) {
    $collection->insert(array("x" => $i), array("w" => 0));
}

$pipeline = array(
    array('$match' => array("x" => array('$gt' => 500))),
);

$cursor = $collection->aggregateCursor($pipeline, array("cursor" => array("batchSize" => 0)));

try {
    foreach ($cursor as $row) {
        // getmore should time out within 1ms, so this should never be reached
        echo "TEST FAILED\n";
    }
} catch(Exception $e) {
    echo $e->getMessage(), "\n";
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
log_getmore
%s:%d: Read timed out after reading 0 bytes, waited for 0.%d seconds
===DONE===
