--TEST--
GridFS: Testing issues with chunks and reading too much
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$conn = mongo_standalone();
$db   = $conn->selectDb(dbname());
$grid = $db->getGridFs('wrapper');

// delete any previous results
$grid->drop();

// dummy file
$bytes = str_repeat("x", 128);
$grid->storeBytes($bytes, array("filename" => "demo.txt", 'chunkSize' => 128), array('w' => true));
unset($bytes);

// fetch it
$file = $grid->findOne(array('filename' => 'demo.txt'));
$chunkSize = $file->file['chunkSize'];

$readSizes = array( 8, 16, 31, 32, 33, 127, 128, 129 );

foreach ($readSizes as $size) {
	$fp = $file->getResource();
	while (!feof($fp)) {
		$t = fread($fp, $size);
		if ($size != strlen($t)) {
			echo "read size(", strlen($t), ") is not the same as requested size ($size)\n";
		} else {
			echo "read: ", strlen($t), " bytes\n";
		}
	}
	fclose($fp);
}
?>
--EXPECT--
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 8 bytes
read: 16 bytes
read: 16 bytes
read: 16 bytes
read: 16 bytes
read: 16 bytes
read: 16 bytes
read: 16 bytes
read: 16 bytes
read: 31 bytes
read: 31 bytes
read: 31 bytes
read: 31 bytes
read size(4) is not the same as requested size (31)
read: 32 bytes
read: 32 bytes
read: 32 bytes
read: 32 bytes
read: 33 bytes
read: 33 bytes
read: 33 bytes
read size(29) is not the same as requested size (33)
read: 127 bytes
read size(1) is not the same as requested size (127)
read: 128 bytes
read size(128) is not the same as requested size (129)
