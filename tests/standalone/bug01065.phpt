--TEST--
Test for PHP-1065: Mongo driver is crashing during getmore
--SKIPIF--
<?php $needs = "2.6.0"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
function log_getmore($server, $cursor_options)
{
    echo __METHOD__, "\n";
}

$ctx = stream_context_create(
    array(
        "mongodb" => array( "log_getmore" => "log_getmore",)
    )
);

require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host . "/test?socketTimeoutMS=10", array(), array("context" => $ctx));
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();



foreach(range(0, 8196) as $i) {
    $collection->insert(array("document" => $i, uniqid("aggr") => uniqid(), "sum" => $i*10), array("w" => 0));
}

$pipeline = array(array(
    '$match' => array("document" => array('$gt' => 3030)),
));

$cursor = $collection->aggregateCursor($pipeline, array("cursor" => array("batchSize" => 0)));
$x = 0;
try {
    foreach($cursor as $row) {
        echo "TEST FAILED\n";
    }
    echo "The getmore should have timedout :( your system may have super powers?\n";
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

