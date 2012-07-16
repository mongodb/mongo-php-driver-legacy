--TEST--
Test for PHP-207: setSlaveOkay not supported for GridFS queries
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";

// FIXME: How on earth can we verify the data came from secondaries?
// The only way I found so far was to watch mongotop :)
$m = mongo("phpunit");
$db = $m->selectDB("phpunit");
$db->dropCollection("fs.files");
$db->dropCollection("fs.chunks");

$gridfs = $db->getGridFS();

for($i=0; $i<10; $i++) {
    $x = $i . rand(0, 10);
    $ok = $gridfs->storeFile(__FILE__, array( "_id" => "slaveOkayFile-$x"));
    var_dump($ok);
}

while($i--) {
    $cursor = $gridfs->find()->slaveOkay(true);
    foreach($cursor as $file) {
        var_dump(strlen($file->getBytes()) > 500);
    }
}

?>
--EXPECTF--
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
