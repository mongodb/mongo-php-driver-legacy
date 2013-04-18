--TEST--
MongoCursor::__construct() error with non-string field names in projection
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo_standalone();
$c = $m->selectCollection(dbname(), 'mongocursor-construct_error-001');

$c->drop();
$c->insert(array('123' => 0));

try {
    $document = $c->findOne(array(), array('_id' => 0, '123' => 1));
    var_dump($document);
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

try {
    $document = $c->findOne(array(), (object) array('_id' => 0, '123' => 1));
    var_dump($document);
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECT--
string(27) "field names must be strings"
int(8)
array(1) {
  [123]=>
  int(0)
}
