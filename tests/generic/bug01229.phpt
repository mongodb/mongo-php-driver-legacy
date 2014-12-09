--TEST--
Test for PHP-1229: MongoGridFS::remove() ignores justOne option when deleting chunks
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$db = $mc->selectDB(dbname());
$gridfs = $db->getGridFS(collname(__FILE__));
$gridfs->drop();

echo "Inserting 3 files\n";

$gridfs->storeBytes('foo', array('x' => 1));
$gridfs->storeBytes('bar', array('x' => 2));
$gridfs->storeBytes('baz', array('x' => 2));

printf("Counting fs.files documents: %d\n", $gridfs->count());
printf("Counting fs.chunks documents: %d\n", $gridfs->chunks->count());

echo "\nRemoving just one file matching {x:2}\n";

$gridfs->remove(array('x' => 2), array('justOne' => true));

printf("Counting fs.files documents: %d\n", $gridfs->count());
printf("Counting fs.chunks documents: %d\n", $gridfs->chunks->count());

?>
===DONE===
--EXPECT--
Inserting 3 files
Counting fs.files documents: 3
Counting fs.chunks documents: 3

Removing just one file matching {x:2}
Counting fs.files documents: 2
Counting fs.chunks documents: 2
===DONE===
