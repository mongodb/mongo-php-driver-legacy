--TEST--
Test for bug PHP-343: Segfault when adding a file to GridFS (storeFile)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";
$m = mongo();
$db = $m->phpunit;
$db->dropCollection( 'phpunit' );
$grid = $db->getGridFS();
$grid->drop();
$saved = $grid->storeFile(
	__FILE__,
	array(
		'filename' => 'test_file-'.rand(0,10000),
		'thumbnail_size' => 'm',
		'otherdata' => 'BIG'
	),
	array('safe' => true)
);
var_dump( $grid->findOne() );
echo "OK\n";
?>
--EXPECTF--
object(MongoGridFSFile)#%d (%d) {
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
      int(%d)
      ["usec"]=>
      int(%d)
    }
    ["length"]=>
    int(%d)
    ["chunkSize"]=>
    int(262144)
    ["md5"]=>
    string(32) "%s"
  }
  ["gridfs%S:protected%S]=>
  object(MongoGridFS)#3 (5) {
    ["w"]=>
    int(1)
    ["wtimeout"]=>
    int(10000)
    ["chunks"]=>
    object(MongoCollection)#4 (2) {
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
