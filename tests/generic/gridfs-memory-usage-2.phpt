--TEST--
GridFS: Testing memory usage
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
for ($i=0; $i < 1024*1024; $i++) {
    $bytes .= sha1(rand(1, 1000000000));
}
$sha = sha1($bytes);
$grid->storeBytes($bytes, array("filename" => "demo.txt"), array('w' => true));

$memory = memory_get_usage();
echo $memory, "\n";
echo $memory > 40 * 1048576 ? "true" : "false", "\n";
echo $memory < 43 * 1048576 ? "true" : "false", "\n";
unset($bytes);

// fetch it
$file = $grid->findOne(array('filename' => 'demo.txt'));
$chunkSize = $file->file['chunkSize'];

// get file descriptor
$fp = $file->getResource();

$tmp = "";
$i=0;
while (!feof($fp)) {
	$s = 500000; // request 500k, but never more than 8192 is read
    $t = fread($fp, $s);
	$i += strlen($t);
}

$memory = memory_get_usage();
echo $memory, "\n";
echo $memory > 1 * 1048576 ? "true" : "false", "\n";
echo $memory < 4 * 1048576 ? "true" : "false", "\n";

$memory = memory_get_peak_usage();
echo $memory, "\n";
echo $memory > 40 * 1048576 ? "true" : "false", "\n";
echo $memory < 43 * 1048576 ? "true" : "false", "\n";


fclose($fp);
echo $i, "\n";
?>
--EXPECTF--
%d
true
true
%d
true
true
%d
true
true
41943040
