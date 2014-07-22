--TEST--
Mongo::lastError()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--INI--
error_reporting=E_ALL & ~E_DEPRECATED
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
$m = new Mongo($host, array(), array('context' => $ctx));

$m->lastError();

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
Issuing getlasterror command on namespace: admin.$cmd
===DONE===
