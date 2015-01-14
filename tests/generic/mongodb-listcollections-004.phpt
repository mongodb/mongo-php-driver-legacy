--TEST--
MongoDB::listCollections() with "filter" option (legacy mode)
--SKIPIF--
<?php $needs = "2.7.5"; $needsOp = "<"; ?>
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

$db->selectCollection('profile')->drop();
$db->createCollection('profile');

$db->selectCollection('capped')->drop();
$db->createCollection('capped', array('capped' => true, 'size' => 1000));

// Convert MongoCollections to name strings, filter relevant names, and sort
function prepareCollectionNames(array $collections) {
    $names = array_filter(
        array_map(function(MongoCollection $c) { return $c->getName(); }, $collections),
        function($name) { return in_array($name, array('system.profile', 'profile', 'capped')); }
    );

    sort($names);

    return $names;
}

$collections = $db->listCollections(array(
    'includeSystemCollections' => true,
    'filter' => array(
        'options.capped' => true,
    ),
));

var_dump(prepareCollectionNames($collections));

$collections = $db->listCollections(array(
    'includeSystemCollections' => true,
    'filter' => array(
        'name' => 'profile',
    ),
));

var_dump(prepareCollectionNames($collections));

?>
===DONE===
--EXPECT--
array(2) {
  [0]=>
  string(6) "capped"
  [1]=>
  string(14) "system.profile"
}
array(1) {
  [0]=>
  string(7) "profile"
}
===DONE===
