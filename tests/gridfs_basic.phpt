--TEST--
MongoGridFS class - basic file functionality
--FILE--
<?php 

include_once "Mongo.php";

$filename = "./tests/somefile";
$outfile = "./anotherfile";

$m = new Mongo();
$db = $m->selectDB('phpt');

$files = $db->selectCollection('fs.files');
$chunks = $db->selectCollection('fs.chunks');

$grid = $db->getGridFS();
$grid->drop();

var_dump($files->count());
var_dump($chunks->count());

var_dump(filesize($filename));

$grid->storeFile($filename);

$file = $grid->getFile(array("filename" => $filename));
var_dump($file->getFilename());
var_dump($file->getSize());

$file->write($outfile);
var_dump(filesize($outfile));

//unlink($outfile);

?>
--EXPECTF--
int(0)
int(0)
int(5183739)
string(16) "./tests/somefile"
int(5183739)
int(5183739)
