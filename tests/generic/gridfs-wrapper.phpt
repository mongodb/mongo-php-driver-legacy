--TEST--
GridFS: Test for basic stream wrapper support
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$conn = new MongoClient($dsn);
$db   = $conn->selectDb('admin');
$grid = $db->getGridFs('wrapper');

// delete any previous results
$grid->drop();

// dummy file
$bytes = "";
for ($i=0; $i < 200*1024; $i++) {
    $bytes .= sha1(rand(1, 1000000000));
}
$grid->storeBytes($bytes, array("filename" => "demo.txt"));

// fetch it
$file = $grid->findOne(array('filename' => 'demo.txt'));

// get file descriptor
$fp = $file->getResource();
/**/
var_dump($fp);
var_dump(fstat($fp));
var_dump(substr($bytes,0,1024) === fread($fp, 1024));
var_dump(feof($fp) === false);


--EXPECTF--
resource(%d) of type (stream)
array(26) {
  [0]=>
  int(0)
  [1]=>
  int(0)
  [2]=>
  int(0)
  [3]=>
  int(0)
  [4]=>
  int(0)
  [5]=>
  int(0)
  [6]=>
  int(0)
  [7]=>
  int(%d)
  [8]=>
  int(0)
  [9]=>
  int(0)
  [10]=>
  int(0)
  [11]=>
  int(0)
  [12]=>
  int(0)
  ["dev"]=>
  int(0)
  ["ino"]=>
  int(0)
  ["mode"]=>
  int(0)
  ["nlink"]=>
  int(0)
  ["uid"]=>
  int(0)
  ["gid"]=>
  int(0)
  ["rdev"]=>
  int(0)
  ["size"]=>
  int(%d)
  ["atime"]=>
  int(0)
  ["mtime"]=>
  int(0)
  ["ctime"]=>
  int(0)
  ["blksize"]=>
  int(0)
  ["blocks"]=>
  int(0)
}
bool(true)
bool(true)
