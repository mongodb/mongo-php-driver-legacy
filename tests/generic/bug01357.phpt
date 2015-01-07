--TEST--
Test for PHP-1357: Command cursor may cause log_response_header callback to segfault
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

function log_response_header($server, $query, $info) {
    echo 'Response header received, query is ' . ($query === null ? '' : 'not ') . "null\n";
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_query' => 'log_query',
        'log_getmore' => 'log_getmore',
        'log_response_header' => 'log_response_header',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

// Ensure there is at least one collection in the test database
$db = $mc->selectDB(dbname());
$db->selectCollection(collname(__FILE__))->drop();
$db->createCollection(collname(__FILE__));

$db->listCollections(array(
    'cursor' => array('batchSize' => 0),
));

?>
===DONE===
--EXPECT--
Issuing command: drop
Response header received, query is not null
Issuing command: create
Response header received, query is not null
Issuing command: listCollections
Cursor batch size: 0
Response header received, query is not null
Issuing getmore
Response header received, query is null
===DONE===
