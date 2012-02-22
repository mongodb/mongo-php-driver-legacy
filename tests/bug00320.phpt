--TEST--
Test for bug PHP-320: GridFS transaction issues with storeFile().
--FILE--
<?php
	$ip = '127.0.0.1';
	$port = '27017';
	$db_name = 'phpunit';
	$m = new Mongo('mongodb://'.$ip.':'.$port.'/'.$db_name);
	$mdb = $m->selectDB($db_name);
	$mdb->dropCollection("fs.files");
	$mdb->dropCollection("fs.chunks");

	$GridFS = $mdb->getGridFS();

	$GridFS->ensureIndex(array('filename' => 1), array("unique" => true));

	$temporary_file_name = '/tmp/GridFS_test.txt';
	$temporary_file_data = '1234567890';
	file_put_contents($temporary_file_name, $temporary_file_data);

	echo "######################################\n";
	echo "# Saving files to GridFS\n";
	echo "######################################\n";
	$options = array( 'safe' => false );
	for ($i = 0; $i < 3; $i++) {
		try {
			$new_saved_file_object_id = $GridFS->storeFile($temporary_file_name, array( '_id' => "file{$i}"), $options);
			echo "[Saved file] New file id:".$new_saved_file_object_id."\n";
		}
		catch (MongoException $e) {
			echo "error message: ".$e->getMessage()."\n";
			echo "error code: ".$e->getCode()."\n";
		}

	}
	var_dump( $options );

	echo "\n";
	echo "######################################\n";
	echo "# Current documents in fs.files\n";
	echo "######################################\n";
	$cursor = $GridFS->findOne('/tmp/GridFS_test.txt');
		foreach($cursor as $this_cursor){
		echo "[file] [_id:".$this_cursor['_id']."] [filename:".$this_cursor['filename']."] [length:".$this_cursor['length']."] [chunkSize:".$this_cursor['chunkSize']."]\n";
	}
	echo "\n";
	echo "######################################\n";
	echo "# Current documents in fs.chunks\n";
	echo "######################################\n";
	$cursor = $GridFS->chunks->find();
	foreach($cursor as $this_cursor){
		echo "[chunk] [_id:".$this_cursor['_id']."] [n:".$this_cursor['n']."] [files_id:".$this_cursor['files_id']."]\n";
	}
?>
--EXPECTF--
######################################
# Saving files to GridFS
######################################
[Saved file] New file id:file0
error message: Could not store file: E11000 duplicate key error index: phpunit.fs.files.$filename_1  dup key: { : "/tmp/GridFS_test.txt" }
error code: 0
error message: Could not store file: E11000 duplicate key error index: phpunit.fs.files.$filename_1  dup key: { : "/tmp/GridFS_test.txt" }
error code: 0
array(1) {
  ["safe"]=>
  bool(false)
}
######################################
# Current documents in fs.files
######################################
[file] [_id:file0] [filename:/tmp/GridFS_test.txt] [length:10] [chunkSize:262144]

######################################
# Current documents in fs.chunks
######################################
[chunk] [_id:%s] [n:0] [files_id:file0]
