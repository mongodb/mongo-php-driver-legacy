--TEST--
Test for bug PHP-343: Segfault when adding a file to GridFS (storeFile)
--FILE--
<?php
$m = new Mongo(); // i.e. a remote host
$db = $m->phpunit;
$db->dropCollection( 'phpunit' );
$grid = $db->getGridFS();
$saved = $grid->storeFile(
	'tests/bug00343-2.phpt',
	array(
		'filename' => 'test_file-'.rand(0,10000),
		'size' => 'm',
		'otherdata' => 'BIG'
	),
	array('safe' => true)
);
var_dump( $grid->findOne() );
echo "OK\n";
?>
--EXPECTF--
object(MongoGridFSFile)#6 (2) {
  ["file"]=>
  array(6) {
    ["_id"]=>
    string(5) "file0"
    ["filename"]=>
    string(9) "file0.txt"
    ["uploadDate"]=>
    object(MongoDate)#7 (2) {
      ["sec"]=>
      int(%d)
      ["usec"]=>
      int(%d)
    }
    ["length"]=>
    int(10)
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
