--TEST--
MongoCode class - basic code functionality
--FILE--
<?php
include "Mongo.php";
$m=new Mongo();
$c = $m->selectCollection("x", "y");
$c->drop();

$code = new MongoCode("if(x<5){ return true; } else { return false;}", array());
$c->insert(array("something" => $code));
var_dump($c->findOne());

$c->remove();
$code = new MongoCode("if(x<5){ return true; } else { return false;}", array("x" => 2));
$c->insert(array("something" => $code));
var_dump($c->findOne());

$c->remove();
$code = new MongoCode("if(x<5){ return true; } else { return false;}");
$c->insert(array("something" => $code));
var_dump($c->findOne());

?>
--EXPECTF--
array(2) {
  ["_id"]=>
  object(MongoId)#5 (1) {
    ["id"]=>
    string(24) "%x"
  }
  ["something"]=>
  object(MongoCode)#6 (2) {
    ["code"]=>
    string(45) "if(x<5){ return true; } else { return false;}"
    ["scope"]=>
    array(0) {
    }
  }
}
array(2) {
  ["_id"]=>
  object(MongoId)#4 (1) {
    ["id"]=>
    string(24) "%x"
  }
  ["something"]=>
  object(MongoCode)#5 (2) {
    ["code"]=>
    string(45) "if(x<5){ return true; } else { return false;}"
    ["scope"]=>
    array(1) {
      ["x"]=>
      int(2)
    }
  }
}
array(2) {
  ["_id"]=>
  object(MongoId)#6 (1) {
    ["id"]=>
    string(24) "%x"
  }
  ["something"]=>
  object(MongoCode)#4 (2) {
    ["code"]=>
    string(45) "if(x<5){ return true; } else { return false;}"
    ["scope"]=>
    array(0) {
    }
  }
}
