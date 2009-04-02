--TEST--
MongoCursor class - iterator functionality
--FILE--
<?php

include "Mongo.php";

$m = new Mongo();
$c = $m->selectCollection("phpt", "cursor");
$c->drop();

for($i=0;$i<50;$i++) {
  $c->insert(array("foo" => $i, 'date' => new MongoDate()));
}

$cursor = $c->find()->limit(4);
foreach($cursor as $k => $v) {
  echo "$k\n";
}

$cursor->reset();
while($x = $cursor->getNext()) {
  unset($x['_id']);
  unset($x['date']);
  var_dump($x);
}

$cursor = $c->find()->limit(1);
foreach($cursor as $v) {
  var_dump($v);
}

?>
--EXPECTF--
%x
%x
%x
%x
array(1) {
  ["foo"]=>
  int(0)
}
array(1) {
  ["foo"]=>
  int(1)
}
array(1) {
  ["foo"]=>
  int(2)
}
array(1) {
  ["foo"]=>
  int(3)
}
array(3) {
  ["_id"]=>
  object(MongoId)#10 (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["foo"]=>
  int(0)
  ["date"]=>
  object(MongoDate)#5 (2) {
    ["sec"]=>
    int(%i)
    ["usec"]=>
    int(%i)
  }
}
