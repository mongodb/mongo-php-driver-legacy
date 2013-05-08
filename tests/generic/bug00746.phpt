--TEST--
Test for PHP-746: The getBytes method may return random data in particular conditions.
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();

$expected = file_get_contents(__FILE__);

/* Store file and remember id */
$id = $gridfs->storeBytes($expected);

/* Remove all chunks, but leave meta data */
$chunks = $db->selectCollection('fs.chunks')->remove();

/* Find file */
$gridfsFile = $gridfs->get($id);

/* Compare expected with returned data (empty) */
try {
	var_dump($gridfsFile->getBytes());
	echo "It should have failed.\n";
} catch( Exception $e ) {
	echo $e->getMessage(), "\n";
}
?>
--EXPECT--
error reading chunk of file
