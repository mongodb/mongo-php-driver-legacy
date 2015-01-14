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
ksort($collections);

foreach ($collections as $name => $info) {
    if ($name != $info['name']) {
        printf("FAILED: '%s' does not match '%s'\n", $name, $info['name']);
    }

    if ($name == 'system.profile' || $name == 'test') {
        echo $name, "\n";
    }

    /* We don't assert the structure of the $info array as it may vary between
     * server versions (e.g. 2.8+ may include a "flags" option).
     */
}

echo "\nTesting with includeSystemCollections=true:\n";

$collections = $db->getCollectionInfo(array('includeSystemCollections' => true));
ksort($collections);

foreach ($collections as $name => $info) {
    if ($name != $info['name']) {
        printf("FAILED: '%s' does not match '%s'\n", $name, $info['name']);
    }

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
