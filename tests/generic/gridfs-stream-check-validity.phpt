--TEST--
GridFS: Testing file validity
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php require_once "tests/utils/standalone.inc" ?>
--INI--
memory_limit=1G
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$conn = new MongoClient($dsn);
$db   = $conn->selectDb('phpunit');
$grid = $db->getGridFs('wrapper');

// delete any previous results
$grid->drop();

// dummy file
$bytes = "";
for ($i=0; $i < 5*1024*1024; $i++) {
    $bytes .= sha1(rand(1, 1000000000));
}
$sha = sha1($bytes);
$grid->storeBytes($bytes, array("filename" => "demo.txt"), array('w' => true));
unset($bytes);

// fetch it
$file = $grid->findOne(array('filename' => 'demo.txt'));
$chunkSize = $file->file['chunkSize'];

// get file descriptor
$fp = $file->getResource();

$tmp = "";
$i=0;
while (!feof($fp)) {
	$s = 500000;
    $t = fread($fp, $s);
	$tmp .= $t;
	$i += strlen($t);
}
fclose($fp);
echo $i, "\n";
if (sha1($tmp) != $sha) {
	echo "files didn't match\n";
}
?>
--EXPECTF--
209715200
