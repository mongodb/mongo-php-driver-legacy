--TEST--
MongoClient::dropDB() with MongoDB argument
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_query($server, $query, $info) {
    printf("Issuing %s command on namespace: %s\n", key($query), $info['ns']);
}

$ctx = stream_context_create(array(
    'mongodb' => array('log_query' => 'log_query'),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));
$db = $mc->selectDB('test_mongoclient_dropdb');

$mc->dropDB($db);

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
Issuing dropDatabase command on namespace: test_mongoclient_dropdb.$cmd
===DONE===
