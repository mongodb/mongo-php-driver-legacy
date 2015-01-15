--TEST--
MongoDB::getCollectionNames()
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
$db->selectCollection('test')->insert(array('_id' => 1));

echo "Testing with includeSystemCollections=false:\n";

$collections = $db->getCollectionNames();
sort($collections);

foreach ($collections as $name) {
    if ($name == 'system.profile' || $name == 'test') {
        echo $name, "\n";
    }
}

echo "\nTesting with includeSystemCollections=true:\n";

$collections = $db->getCollectionNames(array('includeSystemCollections' => true));
sort($collections);

foreach ($collections as $name) {
    if ($name == 'system.profile' || $name == 'test') {
        echo $name, "\n";
    }
}

?>
===DONE===
--EXPECTF--
Testing with includeSystemCollections=false:
test

Testing with includeSystemCollections=true:
system.profile
test
===DONE===
