--TEST--
MongoCursor::__construct() with projection argument and numeric field names
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
    $document = $collection->findOne(array(), array('1' => 1));
    var_dump($document);
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

/* If the $fields array would be indistinguishable from a numeric array, the
 * user must either re-order the keys (assuming multiple fields are projected)
 * or cast $fields to an object. This work-around is necessary due to legacy
 * argument support (i.e. array of field names to be included).
 */
try {
    $document = $collection->findOne(array(), array('1' => 1, '0' => 1));
    var_dump($document);
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

try {
    $document = $collection->findOne(array(), (object) array('0' => 1, '1' => 1));
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
  [1]=>
  string(3) "one"
}
array(3) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  [0]=>
  string(4) "zero"
  [1]=>
  string(3) "one"
}
array(3) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  [0]=>
  string(4) "zero"
  [1]=>
  string(3) "one"
}
