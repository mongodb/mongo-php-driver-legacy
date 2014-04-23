--TEST--
Test for PHP-333: GridFS find's key without returning _id.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone("phpunit");
$mdb = $m->selectDB("phpunit");
$mdb->dropCollection("fs.files");
$mdb->dropCollection("fs.chunks");

$GridFS = $mdb->getGridFS();

$temporary_file_name = '/tmp/GridFS_test.txt';
$temporary_file_data = '1234567890';
file_put_contents($temporary_file_name, $temporary_file_data);

$options = array( 'w' => false );
for ($i = 0; $i < 3; $i++) {
	try {
		$new_saved_file_object_id = $GridFS->storeFile($temporary_file_name, array( '_id' => "file{$i}", "filename" => "file.txt"), $options);
		echo "[Saved file] New file id:".$new_saved_file_object_id."\n";
	}
	catch (MongoException $e) {
		echo "error message: ".$e->getMessage()."\n";
		echo "error code: ".$e->getCode()."\n";
	}

}

echo "\n";
$cursor = $GridFS->find( array(), array( '_id' => 0 ));
foreach ( $cursor as $key => $item )
{
	echo $key, ': ', $item->file['filename'], "\n";
}
echo "\n";

foreach ( iterator_to_array( $cursor ) as $key => $item )
{
	echo $key, ': ', $item->file['filename'], "\n";
}
?>
--EXPECT--
[Saved file] New file id:file0
[Saved file] New file id:file1
[Saved file] New file id:file2

0: file.txt
1: file.txt
2: file.txt

0: file.txt
1: file.txt
2: file.txt
