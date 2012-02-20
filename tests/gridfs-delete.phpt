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
Using ID:string(5) "file0"
Items in DB: 1
Items in DB: 0

Using ID:int(452)
Items in DB: 1
Items in DB: 0

Using ID:bool(true)
Items in DB: 1
Items in DB: 0

Using ID:object(MongoId)#5 (1) {
  ["$id"]=>
  string(24) "%s"
}
Items in DB: 1
Items in DB: 0

Using ID:array(2) {
  [0]=>
  string(1) "a"
  ["b"]=>
  int(5)
}
Items in DB: 1
Items in DB: 0
