--TEST--
MongoDBRef::create()
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php

$ids = array(
    new MongoId('5202e48be84df152458b4567'),
    123,
    'foo',
    array('x' => 1, 'y' => 2),
    (object) array('x' => 1, 'y' => 2),
);

foreach ($ids as $id) {
    var_dump(MongoDBRef::create('collection', $id));
    var_dump(MongoDBRef::create('collection', $id, 'database'));
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
array(3) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5202e48be84df152458b4567"
  }
  ["$db"]=>
  string(8) "database"
}

array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  int(123)
}
array(3) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  int(123)
  ["$db"]=>
  string(8) "database"
}

array(2) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  string(3) "foo"
}
array(3) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  string(3) "foo"
  ["$db"]=>
  string(8) "database"
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
array(3) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  array(2) {
    ["x"]=>
    int(1)
    ["y"]=>
    int(2)
  }
  ["$db"]=>
  string(8) "database"
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
array(3) {
  ["$ref"]=>
  string(10) "collection"
  ["$id"]=>
  object(stdClass)#%d (2) {
    ["x"]=>
    int(1)
    ["y"]=>
    int(2)
  }
  ["$db"]=>
  string(8) "database"
}
