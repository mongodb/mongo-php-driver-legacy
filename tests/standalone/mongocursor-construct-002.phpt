--TEST--
MongoCursor::__construct() with legacy projection argument and numeric field names
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));

$collection->drop();
$collection->insert(array('0' => 'zero', '1' => 'one'));

try {
    $document = $collection->findOne(array(), array('0'));
    var_dump($document);
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  [0]=>
  string(4) "zero"
}
