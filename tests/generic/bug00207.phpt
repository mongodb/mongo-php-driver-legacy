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

for($i=0; $i<5; $i++) {
    $ok = $gridfs->storeFile(__FILE__, array( "_id" => "slaveOkayFile-$i"));
    var_dump($ok);
}
$bytes = strlen(file_get_contents(__FILE__));

$cursor = $gridfs->find()->slaveOkay(true);
var_dump($cursor->count());
foreach($cursor as $file) {
    var_dump($file->file["_id"]);
}
?>
===DONE===
--EXPECTF--
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
string(%d) "%s"
int(5)
string(15) "slaveOkayFile-0"
string(15) "slaveOkayFile-1"
string(15) "slaveOkayFile-2"
string(15) "slaveOkayFile-3"
string(15) "slaveOkayFile-4"
===DONE===
