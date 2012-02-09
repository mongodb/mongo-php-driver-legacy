--TEST--
Test for bug PHP-310: GridFS transaction issues
--FILE--
<?php
	$ip = '127.0.0.1';
	$port = '27017';
	$db_name = 'phpunit';
	$m = new Mongo('mongodb://'.$ip.':'.$port.'/'.$db_name);
	$mdb = $m->selectDB($db_name);

	$GridFS = $mdb->getGridFS();

	$GridFS->ensureIndex(array('filename' => 1), array("unique" => true));

	$temporary_file_name = '/tmp/GridFS_test.txt';
	$temporary_file_data = '1234567890';
	file_put_contents($temporary_file_name, $temporary_file_data);

	echo "######################################\n";
	echo "# Saving files to GridFS\n";
	echo "######################################\n";
	for ($i = 0; $i < 3; $i++) {
		try {
			$new_saved_file_object_id = $GridFS->storeFile($temporary_file_name, array( '_id' => "file{$i}"), array('options' => 'safe'));
		}
		catch (MongoCursorException $e) {
			echo "error message: ".$e->getMessage()."\n";
			echo "\nerror code: ".$e->getCode()."\n";
		}

		echo "[Saved file] New file id:".$new_saved_file_object_id."\n";
	}

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
[Saved file] New file id:file1
[Saved file] New file id:file2

######################################
# Current documents in fs.files
######################################
[file] [_id:file0] [filename:/tmp/GridFS_test.txt] [length:10] [chunkSize:262144]

######################################
# Current documents in fs.chunks
######################################
[chunk] [_id:%s] [n:0] [files_id:file0]
