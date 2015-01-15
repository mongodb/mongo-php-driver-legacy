--TEST--
MongoDB::getCollectionInfo()
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

$collections = $db->getCollectionInfo();
usort($collections, function($a, $b) { return strcmp($a['name'], $b['name']); });

foreach ($collections as $info) {
    if ($info['name'] == 'system.profile' || $info['name'] == 'test') {
        echo $info['name'], "\n";
    }

    /* We don't assert the structure of the $info array as it may vary between
     * server versions (e.g. 2.8+ may include a "flags" option).
     */
}

echo "\nTesting with includeSystemCollections=true:\n";

$collections = $db->getCollectionInfo(array('includeSystemCollections' => true));
usort($collections, function($a, $b) { return strcmp($a['name'], $b['name']); });

foreach ($collections as $info) {
    if ($info['name'] == 'system.profile' || $info['name'] == 'test') {
        echo $info['name'], "\n";
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
