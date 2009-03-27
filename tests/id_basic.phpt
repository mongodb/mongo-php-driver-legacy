--TEST--
_id - basic db id functionality
--FILE--
<?php

$id1 = new MongoId();
var_dump($id1);
$id2 = new MongoId('49c10bb63eba810c0c3fc158');
var_dump($id2);

include "Mongo.php";

$m=new Mongo();
$c = $m->selectCollection("phpt", "id");
$c->drop();
$c->ensureIndex(array("_id"=>1));

$c->insert(array("_id"=>1));
var_dump($c->findOne());

$c->insert(array("_id"=>1));
echo "inserted\n";
$cursor = $c->find();
while($cursor->hasNext()) {
  var_dump($cursor->next());
}

?>
--EXPECTF--
object(MongoId)#1 (1) {
  ["id"]=>
  string(12) "%s"
}
object(MongoId)#2 (1) {
  ["id"]=>
  string(12) "%s"
}
array(1) {
  ["_id"]=>
  int(1)
}
inserted
array(1) {
  ["_id"]=>
  int(1)
}
