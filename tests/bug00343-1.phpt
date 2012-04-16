--TEST--
Test for bug PHP-343: Segfault when adding a file to GridFS (storeBytes)
--FILE--
<?php
$bytes = file_get_contents('tests/bug00343-1.phpt');
$m = new Mongo(); // i.e. a remote host
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
	array('safe' => true)
);
var_dump( $grid->findOne() );
echo "OK\n";
?>
--EXPECTF--
object(MongoGridFSFile)#%d (2) {
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
    object(MongoDate)#9 (2) {
      ["sec"]=>
      int(1%d)
      ["usec"]=>
      int(%d)
    }
    ["length"]=>
    int(1%d)
    ["chunkSize"]=>
    int(262144)
    ["md5"]=>
    string(32) "%s"
  }
  ["gridfs":protected]=>
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
    ["filesName":protected]=>
    string(8) "fs.files"
    ["chunksName":protected]=>
    string(9) "fs.chunks"
  }
}
OK
