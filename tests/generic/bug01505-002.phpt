--TEST--
Test for PHP-1505: GridFS file size should not be limited to 32-bit integer (file resource)
--DESCRIPTION--
This test expects a 5GB file to use for inserting via MongoGridFS::storeFile().
The file may be generated with `fallocate -l 5G /tmp/5gb`.
--SKIPIF--
skip Manual test
<?php if (8 !== PHP_INT_SIZE) { die('skip Only for 64-bit platform'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);
$gridfs = $mc->selectDB(dbname())->getGridFS();
$gridfs->drop();
$fp = fopen('/tmp/5gb', 'r');
$id = $gridfs->storeFile($fp, array('chunkSize' => 4194304));
var_dump(fclose($fp));
$file = $gridfs->get($id);
var_dump($file->file);

?>
==DONE==
--CLEAN--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$mc->selectDB(dbname())->getGridFS()->drop();

?>
--EXPECTF--
bool(true)
array(5) {
  ["_id"]=>
  object(MongoId)#%d (%d) {
    ["$id"]=>
    string(24) "%x"
  }
  ["chunkSize"]=>
  int(4194304)
  ["uploadDate"]=>
  object(MongoDate)#%d (%d) {
    ["sec"]=>
    int(%d)
    ["usec"]=>
    int(%d)
  }
  ["length"]=>
  int(5368709120)
  ["md5"]=>
  string(32) "%x"
}
==DONE==
