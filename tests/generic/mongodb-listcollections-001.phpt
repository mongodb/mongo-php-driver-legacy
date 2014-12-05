--TEST--
MongoDB::listCollections() excludes system collections by default
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDB(dbname());

$db->setProfilingLevel(MongoDB::PROFILING_OFF);
$db->selectCollection('system.profile')->drop();
$db->createCollection('system.profile', array('capped' => true, 'size' => 5000));

$db->selectCollection('test')->drop();
$db->createCollection('test');

// Convert MongoCollections to name strings, filter relevant names, and sort
function prepareCollectionNames(array $collections) {
    $names = array_filter(
        array_map(function(MongoCollection $c) { return $c->getName(); }, $collections),
        function($name) { return in_array($name, array('system.profile', 'test')); }
    );

    sort($names);

    return $names;
}

var_dump(prepareCollectionNames($db->listCollections()));

?>
===DONE===
--EXPECT--
array(1) {
  [0]=>
  string(4) "test"
}
===DONE===
