--TEST--
Test for PHP-1464: GridFS should not drop dupes when creating index
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);
$db = $mc->selectDB(dbname());

$filesId = new MongoId('000000000000000000000002');
$data = new MongoBinData('foo', MongoBinData::BYTE_ARRAY);

// Drop files collection
$filesCollection = $db->selectCollection('fs.files');
$filesCollection->drop();

// Drop chunks collection and insert duplicate, orphan chunks
$chunksCollection = $db->selectCollection('fs.chunks');
$chunksCollection->drop();
$chunksCollection->insert(array(
    '_id' => new MongoId('000000000000000000000002'),
    'files_id' => new MongoId('000000000000000000000001'),
    'n' => 0,
    'data' => new MongoBinData('foo', MongoBinData::BYTE_ARRAY),
));
$chunksCollection->insert(array(
    '_id' => new MongoId('000000000000000000000003'),
    'files_id' => new MongoId('000000000000000000000001'),
    'n' => 0,
    'data' => new MongoBinData('bar', MongoBinData::BYTE_ARRAY),
));

// Test three methods that start by ensuring the unique index
echo "MongoGridFS::storeBytes():\n";

try {
    $db->getGridFS()->storeBytes('foo');
} catch (MongoGridFSException $e) {
    echo $e->getMessage(), "\n";
}

echo "\nMongoGridFS::storeFile():\n";

try {
    $db->getGridFS()->storeFile(__FILE__);
} catch (MongoGridFSException $e) {
    echo $e->getMessage(), "\n";
}

echo "\nMongoGridFS::remove():\n";

try {
    $db->getGridFS()->remove();
} catch (MongoGridFSException $e) {
    echo $e->getMessage(), "\n";
}

echo "\nDumping fs.files:\n";

foreach ($filesCollection->find() as $file) {
    var_dump($file);
}

echo "\nDumping fs.chunks:\n";

foreach ($chunksCollection->find() as $chunk) {
    var_dump($chunk);
}

?>
==DONE==
--CLEAN--
<?php
require_once "tests/utils/server.inc";

// Ensure our duplicate chunks are removed
$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$mc->selectDB(dbname())->getGridFS()->drop();

?>
--EXPECTF--
MongoGridFS::storeBytes():
Could not store file: %s:%d: %SE11000 duplicate key error %s: %sfiles_id%sdup key: { : ObjectId('000000000000000000000001'), : 0 }

MongoGridFS::storeFile():
Could not store file: %s:%d: %SE11000 duplicate key error %s: %sfiles_id%sdup key: { : ObjectId('000000000000000000000001'), : 0 }

MongoGridFS::remove():
Could not store file: %s:%d: %SE11000 duplicate key error %s: %sfiles_id%sdup key: { : ObjectId('000000000000000000000001'), : 0 }

Dumping fs.files:

Dumping fs.chunks:
array(4) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "000000000000000000000002"
  }
  ["files_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "000000000000000000000001"
  }
  ["n"]=>
  int(0)
  ["data"]=>
  object(MongoBinData)#%d (2) {
    ["bin"]=>
    string(3) "foo"
    ["type"]=>
    int(2)
  }
}
array(4) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "000000000000000000000003"
  }
  ["files_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "000000000000000000000001"
  }
  ["n"]=>
  int(0)
  ["data"]=>
  object(MongoBinData)#%d (2) {
    ["bin"]=>
    string(3) "bar"
    ["type"]=>
    int(2)
  }
}
==DONE==
