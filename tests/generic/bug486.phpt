--TEST--
Test for PHP-486: GridFS cleanup routines remove existing files
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc" ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$m = mongo();
$db = $m->selectDB("examples");

$gridfs = $db->getGridFS();
$gridfs->drop();
$gridfs->storeFile("./README.md", array('_id' => 1));
$it = $gridfs->find();
foreach($it as $file) {
    var_dump($file->file["filename"]);
}
try {
    $gridfs->storeFile(__FILE__, array('_id' => 1));
} catch(Exception $e) {
    var_dump($e->getMessage());
}
$it = $gridfs->find();
foreach($it as $file) {
    var_dump($file->file["filename"]);
}
$gridfs->drop();

?>
--EXPECTF--
string(11) "./README.md"
string(91) "E11000 duplicate key error index: %s.fs.chunks.$files_id_1_n_1  dup key: { : 1, : 0 }"
string(11) "./README.md"
