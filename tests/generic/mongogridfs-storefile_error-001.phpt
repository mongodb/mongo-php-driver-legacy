--TEST--
MongoGridFS::storeFile() throws exception if insert fails with safe option
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
$gridfs->drop();
$gridfs->storeFile(__FILE__, array('_id' => 1), array('safe' => true));

try {
    $gridfs->storeFile(__FILE__, array('_id' => 1), array('safe' => true));
    var_dump(false);
} catch (MongoGridFSException $e) {
    var_dump(true);
}
--EXPECT--
bool(true)
