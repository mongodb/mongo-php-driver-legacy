--TEST--
Test for PHP-320: GridFS transaction issues with storeBytes(). (2)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = new_mongo_standalone("phpunit");
	$mdb = $m->selectDB("phpunit");
	$mdb->dropCollection("fs.files");
	$mdb->dropCollection("fs.chunks");

	$GridFS = $mdb->getGridFS();

	$GridFS->ensureIndex(array('filename' => 1), array("unique" => true));

	$temporary_file_data = '1234567890';

	echo "######################################\n";
	echo "# Saving files to GridFS\n";
	echo "######################################\n";
	for ($i = 0; $i < 3; $i++) {
		try {
			$new_saved_file_object_id = $GridFS->storeBytes($temporary_file_data, array( '_id' => "file{$i}", 'filename' => '/tmp/GridFS_test.txt'));
			echo "[Saved file] New file id:".$new_saved_file_object_id."\n";
		}
		catch (MongoException $e) {
			echo "error message: ".$e->getMessage()."\n";
			echo "error code: ".$e->getCode()."\n";
		}

	}

	echo "\n";
	echo "######################################\n";
	echo "# Current documents in fs.files\n";
	echo "######################################\n";
	$cursor = $GridFS->findOne('/tmp/GridFS_test.txt');
    $this_cursor = $cursor->file;
		echo "[file] [_id:".$this_cursor['_id']."] [filename:".$this_cursor['filename']."] [length:".$this_cursor['length']."] [chunkSize:".$this_cursor['chunkSize']."]\n";
	echo "\n";
	echo "######################################\n";
	echo "# Current documents in fs.chunks\n";
	echo "######################################\n";
	$cursor = $GridFS->chunks->find();
    foreach($cursor as $this_cursor) {
		echo "[chunk] [_id:".$this_cursor['_id']."] [n:".$this_cursor['n']."] [files_id:".$this_cursor['files_id']."]\n";
    }
?>
--EXPECTF--
######################################
# Saving files to GridFS
######################################
[Saved file] New file id:file0
error message: Could not store file: %s:%d:%s { : "/tmp/GridFS_test.txt" }
error code: 11000
error message: Could not store file: %s:%d:%s { : "/tmp/GridFS_test.txt" }
error code: 11000

######################################
# Current documents in fs.files
######################################
[file] [_id:file0] [filename:/tmp/GridFS_test.txt] [length:10] [chunkSize:261120]

######################################
# Current documents in fs.chunks
######################################
[chunk] [_id:%s] [n:0] [files_id:file0]
