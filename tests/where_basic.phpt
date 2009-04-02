--TEST--
$where - basic $where functionality
--FILE--
<?php

include "Mongo.php";

$m=new Mongo();
$db = $m->selectDB("phpt");
$c = $db->selectCollection("foo");
$c->drop();

for($i=0;$i<50; $i++) {
  $c->insert(array( "foo$i" => pow(2, $i)));
}

var_dump($c->findOne(array('$where' => new MongoCode('function() { return this.foo23 != null; }'))));

?>
--EXPECTF--
array(2) {
  ["_id"]=>
  object(MongoId)#6 (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["foo23"]=>
  int(8388608)
}
