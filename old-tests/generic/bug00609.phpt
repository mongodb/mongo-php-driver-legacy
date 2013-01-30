--TEST--
Test for PHP-609: MongoGridFS::put not returning MongoId despite inserting file
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$gridfs = $m->selectDb(dbname())->getGridFS();
$retval = $gridfs->put(__FILE__, array("meta" => "data"));
$gridfs->drop();
var_dump($retval);
?>
--EXPECTF--
object(MongoId)#%d (1) {
  ["$id"]=>
  string(24) "%s"
}

