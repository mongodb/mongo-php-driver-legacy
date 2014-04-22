--TEST--
Test for PHP-1085: w=0 can return an unexpected exception on failure
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
$mc = new MongoClient($host . "/test?socketTimeoutMS=1&w=0", array(), array("context" => $ctx));
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
try {
    $collection->drop();
} catch(Exception $e) {
    /* Commands are always roundtrips and can fail with w=0 */
}


$doc = array("document" => -1, uniqid("aggr") => uniqid(), "sum" => -10);
$collection->insert($doc, array("w" => 0));
foreach(range(0, 8196) as $i) {
    $retval = $collection->insert(array("document" => $i, uniqid("aggr") => uniqid(), "sum" => $i*10));
    if (!is_bool($retval)) {
        echo "Insert $i broke, should have been a boolean!\n";
        var_dump($retval);
    }
}
echo "Good, good. No exeptions for w=0 insert, what about remove and update?\n";

$collection->remove($doc["_id"]);
$collection->update($doc["_id"], array("good" => "behaviour"), array("upsert" => true));
echo "Good. Now lets try with w=1\n";
try {
    $collection->remove($doc["_id"], array("w" => 1));
    echo "That should have failed!\n";
} catch(Exception $e) {
    /* This could fail for any number of reasons, depending on how fast your localhost is */
    echo "Got exception\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
Good, good. No exeptions for w=0 insert, what about remove and update?
Good. Now lets try with w=1
Got exception
===DONE===
