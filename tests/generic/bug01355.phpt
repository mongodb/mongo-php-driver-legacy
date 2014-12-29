--TEST--
Test for PHP-1355: Collection enumeration fails if cursor's first batch is empty
--SKIPIF--
<?php $needs = "2.8.0-RC4"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
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
$mc = new MongoClient($host, array(), array('context' => $ctx));

// Ensure there is at least one collection in the test database
$db = $mc->selectDB(dbname());
$db->selectCollection(collname(__FILE__))->drop();
$db->createCollection(collname(__FILE__));

echo "\nTesting listCollections\n";

$collections = $db->listCollections(array(
    'includeSystemCollections' => false,
    'cursor' => array('batchSize' => 0),
));

var_dump(count($collections) >= 1);

/* We cannot test listIndexes unless if supports an $options array, which would
 * allow us to specify cursor and batchSize options for the command.
 */

?>
===DONE===
--EXPECT--
Issuing command: drop
Issuing command: create

Testing listCollections
Issuing command: listCollections
Cursor batch size: 0
Issuing getmore
bool(true)
===DONE===
