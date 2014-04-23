--TEST--
Test for PHP-343: Segfault when adding a file to GridFS (storeBytes).
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = new_mongo_standalone();
$bytes = file_get_contents(__FILE__);
$db = $m->phpunit;
$db->dropCollection( 'phpunit' );
$grid = $db->getGridFS();
$grid->drop();
$saved = $grid->storeBytes(
	$bytes,
	array(
		'filename' => 'test_file-'.rand(0,10000),
		'thumbnail_size' => 'm',
		'otherdata' => 'BIG'
	),
	array('w' => true)
);
var_dump( $grid->findOne() );
echo "OK\n";
?>
--EXPECTF--
object(MongoGridFSFile)#%d (3) {
  ["file"]=>
  array(8) {
    ["_id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["filename"]=>
    string(%d) "test_file-%d"
    ["thumbnail_size"]=>
    string(1) "m"
    ["otherdata"]=>
    string(3) "BIG"
    ["uploadDate"]=>
    object(MongoDate)#%d (2) {
      ["sec"]=>
      int(1%d)
      ["usec"]=>
      int(%d)
    }
    ["length"]=>
    int(%d)
    ["chunkSize"]=>
    int(261120)
    ["md5"]=>
    string(32) "%s"
  }
  ["gridfs%S:protected%S]=>
  object(MongoGridFS)#%d (5) {
    ["w"]=>
    int(1)
    ["wtimeout"]=>
    int(10000)
    ["chunks"]=>
    object(MongoCollection)#%d (2) {
      ["w"]=>
      int(1)
      ["wtimeout"]=>
      int(10000)
    }
    ["filesName%S:protected%S]=>
    string(8) "fs.files"
    ["chunksName%S:protected%S]=>
    string(9) "fs.chunks"
  }
  ["flags"]=>
  int(%d)
}
OK
