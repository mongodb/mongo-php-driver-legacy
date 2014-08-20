--TEST--
MongoCollection::deleteIndex() uses dropIndexes command
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_query($server, $query, $info) {
    var_dump($query);
}

$ctx = stream_context_create(array(
    'mongodb' => array('log_query' => 'log_query'),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->deleteIndex(array('foo' => 1));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->deleteIndex('bar');

?>
===DONE===
--EXPECTF--
array(2) {
  ["dropIndexes"]=>
  string(%d) "%s"
  ["index"]=>
  string(5) "foo_1"
}
array(2) {
  ["dropIndexes"]=>
  string(%d) "%s"
  ["index"]=>
  string(5) "bar_1"
}
===DONE===
