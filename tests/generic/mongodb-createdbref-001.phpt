--TEST--
MongoDB::createDBRef()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$dsn = MongoShellServer::getStandaloneInfo();
$mongo = new MongoClient($dsn);

$ids = array(
    new MongoId('5202e48be84df152458b4567'),
    123,
    'foo',
    array('_id' => array('x' => 1, 'y' => 2)),
    (object) array('_id' => (object) array('x' => 1, 'y' => 2)),
);

foreach ($ids as $id) {
    var_dump($mongo->database->createDBRef('collection', $id));
    echo "\n";
}

?>
--EXPECTF--
array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5202e48be84df152458b4567"
  }
}

array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  int(123)
}

array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  string(3) "foo"
}

array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  array(2) {
    ["x"]=>
    int(1)
    ["y"]=>
    int(2)
  }
}

array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(stdClass)#%d (2) {
    ["x"]=>
    int(1)
    ["y"]=>
    int(2)
  }
}
