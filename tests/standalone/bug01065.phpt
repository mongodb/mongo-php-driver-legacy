--TEST--
Test for PHP-1065: Mongo driver is crashing during getmore
--SKIPIF--
<?php $needs = "2.6.0"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php

function log_getmore($server, $cursor_options)
{
    printf("%s\n", __METHOD__);
}

$ctx = stream_context_create(array(
    "mongodb" => array("log_getmore" => "log_getmore")
));

require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array("context" => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

for ($i = 0; $i < 10; ++$i) {
    $collection->insert(array("x" => $i), array("w" => 0));
}

$cursor = $collection->find(array('$where' => 'sleep(1) || true'));
$cursor->batchSize(2);
$cursor->timeout(-1);

$document = $cursor->getNext();
printf("First document: x = %d\n", $document['x']);

$document = $cursor->getNext();
printf("Second document: x = %d\n", $document['x']);

$cursor->timeout(1);

try {
    $document = $cursor->getNext();
    // getmore should time out within 1ms, so this should never be reached
    echo "Expected getmore to time out but it did not!\n";
} catch(MongoCursorTimeoutException $e) {
    echo $e->getMessage(), "\n";
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
First document: x = 0
Second document: x = 1
log_getmore
%s:%d: Read timed out after reading 0 bytes, waited for 0.%d seconds
===DONE===
