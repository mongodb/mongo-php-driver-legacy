--TEST--
GridFS: deleting files by ID
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

	$temporary_file_name = '/tmp/GridFS_test.txt';
	$temporary_file_data = '1234567890';
	file_put_contents($temporary_file_name, $temporary_file_data);

	$ids = array(
		"file0",
		452,
		true,
		new MongoID(),
		array( 'a', 'b' => 5 ),
	);

	foreach ( $ids as $id )
	{
		echo "Using ID:";
		var_dump( $id );
		$GridFS->storeFile($temporary_file_name, array( '_id' => $id));

		echo "Items in DB: ", $GridFS->find()->count(), "\n";
		$GridFS->delete( $id );
		echo "Items in DB: ", $GridFS->find()->count(), "\n";

		echo "\n";
	}
?>
--EXPECTF--
