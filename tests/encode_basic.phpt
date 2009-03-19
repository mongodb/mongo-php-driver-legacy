--TEST--
bson - basic encode/decode functionality
--FILE--
<?php

include "Mongo.php";

$m=new Mongo();
$c=$m->selectCollection("phpt", "bindata");
$c->drop();

$a = array("n" => NULL, 
           "l" => 234234124,
           "d" => 23.23451452,
           "b" => true,
           "a" => array("foo"=>"bar",
                        "n" => NULL,
                        "x" => new MongoId("49b6d9fb17330414a0c63102")),
           "d2" => new MongoDate(1271079861),
           "regex" => new MongoRegex("/xtz/g"),
           "_id" => new MongoId("49b6d9fb17330414a0c63101"),
           "string" => "string");

$c->insert($a);

var_dump($c->findOne());

?>
--EXPECT--
array(9) {
  ["_id"]=>
  object(MongoId)#12 (1) {
    ["id"]=>
    string(24) "49b6d9fb17330414a0c63101"
  }
  ["n"]=>
  NULL
  ["l"]=>
  int(234234124)
  ["d"]=>
  float(23.23451452)
  ["b"]=>
  int(1)
  ["a"]=>
  array(3) {
    ["foo"]=>
    string(3) "bar"
    ["n"]=>
    NULL
    ["x"]=>
    object(MongoId)#9 (1) {
      ["id"]=>
      string(24) "49b6d9fb17330414a0c63102"
    }
  }
  ["d2"]=>
  object(MongoDate)#10 (2) {
    ["sec"]=>
    int(1271079861)
    ["usec"]=>
    int(0)
  }
  ["regex"]=>
  object(MongoRegex)#11 (2) {
    ["regex"]=>
    string(3) "xtz"
    ["flags"]=>
    string(1) "g"
  }
  ["string"]=>
  string(6) "string"
}
