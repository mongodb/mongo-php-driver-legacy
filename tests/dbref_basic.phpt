--TEST--
MongoDBRef class - basic dbref functionality
--FILE--
<?php
include "mongo.php";

$m=new Mongo();
$db = $m->selectDB("phpt");
$people = $db->selectCollection("people");
$people->drop();
$addresses = $db->selectCollection("addresses");
$addresses->drop();

// insert some addresses
$i1 = array(array("address" => "45 Oakwood Drive",
                  "city" => "Brooklyn",
                  "state" => "New York"),
            array("address" => "150 2nd Ave",
                  "city" => "New York",
                  "state" => "New York"));
$addresses->batchInsert($i1);

$address1 = $addresses->findOne(array("city" => "Brooklyn"));
var_dump($address1);
// create a reference to an address
$person = array("name" => "kristina",
                "address" => $addresses->createDBRef($address1));

$people->insert($person);
$p = $people->findOne();
var_dump($p);

// resolve ref
var_dump($db->getDBRef($p["address"]));
var_dump($people->getDBRef($p["address"]));
var_dump(MongoDBRef::get($db, $p["address"]));
?>
--EXPECTF--
array(4) {
  ["address"]=>
  string(16) "45 Oakwood Drive"
  ["city"]=>
  string(8) "Brooklyn"
  ["state"]=>
  string(8) "New York"
  ["_id"]=>
  object(MongoId)#5 (1) {
    ["id"]=>
    string(24) "%x"
  }
}
array(3) {
  ["_id"]=>
  object(MongoId)#6 (1) {
    ["id"]=>
    string(24) "%x"
  }
  ["name"]=>
  string(8) "kristina"
  ["address"]=>
  array(2) {
    ["$ref"]=>
    string(9) "addresses"
    ["$id"]=>
    object(MongoId)#7 (1) {
      ["id"]=>
      string(24) "%x"
    }
  }
}
array(4) {
  ["address"]=>
  string(16) "45 Oakwood Drive"
  ["city"]=>
  string(8) "Brooklyn"
  ["state"]=>
  string(8) "New York"
  ["_id"]=>
  object(MongoId)#9 (1) {
    ["id"]=>
    string(24) "%x"
  }
}
array(4) {
  ["address"]=>
  string(16) "45 Oakwood Drive"
  ["city"]=>
  string(8) "Brooklyn"
  ["state"]=>
  string(8) "New York"
  ["_id"]=>
  object(MongoId)#8 (1) {
    ["id"]=>
    string(24) "%x"
  }
}
array(4) {
  ["address"]=>
  string(16) "45 Oakwood Drive"
  ["city"]=>
  string(8) "Brooklyn"
  ["state"]=>
  string(8) "New York"
  ["_id"]=>
  object(MongoId)#9 (1) {
    ["id"]=>
    string(24) "%x"
  }
}