--TEST--
Test for PHP-1358: Allow command cursor option to be array or object
--SKIPIF--
<?php $needs = "2.8.0-RC3"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_query($server, $query, $info) {
    printf("Issuing command: %s\n", key($query));

    if (isset($query['cursor'])) {
        echo "Cursor option:\n";
        var_dump($query['cursor']);
    }
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_query' => 'log_query',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$db = $mc->selectDB(dbname());

$collection = $db->selectCollection(collname(__FILE__));
$collection->drop();
$collection->insert(array('x' => 1));

echo "\nTesting aggregate command with empty cursor option:\n";

$cursor = $collection->aggregateCursor(
    array(array('$match' => array('x' => 1))),
    array('cursor' => (object) array())
);
$cursor->rewind();

echo "\nTesting aggregate command with custom batch size:\n";

$cursor = $collection->aggregateCursor(
    array(array('$match' => array('x' => 1))),
    array('cursor' => (object) array('batchSize' => 1))
);
$cursor->rewind();

echo "\nTesting listCollections command with empty cursor option:\n";

$collections = $db->listCollections(
    array('cursor' => (object) array())
);

echo "\nTesting listCollections command with custom batch size:\n";

$collections = $db->listCollections(
    array('cursor' => (object) array('batchSize' => 1))
);

?>
===DONE===
--EXPECTF--
Issuing command: drop

Testing aggregate command with empty cursor option:
Issuing command: aggregate
Cursor option:
object(stdClass)#%d (0) {
}

Testing aggregate command with custom batch size:
Issuing command: aggregate
Cursor option:
object(stdClass)#%d (1) {
  ["batchSize"]=>
  int(1)
}

Testing listCollections command with empty cursor option:
Issuing command: listCollections
Cursor option:
object(stdClass)#%d (0) {
}

Testing listCollections command with custom batch size:
Issuing command: listCollections
Cursor option:
object(stdClass)#%d (1) {
  ["batchSize"]=>
  int(1)
}
===DONE===
