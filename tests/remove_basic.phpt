--TEST--
MongoCollection::remove() - basic remove functionality
--FILE--
<?php
include "Mongo.php";

$m = new Mongo();
$c = $m->selectCollection("phpt", "foo.bar");

$c->drop();

for($i=0;$i<15;$i++) {
  $c->insert(array("i"=>$i));
}

var_dump($c->count());

$c->remove();

var_dump($c->count());
$cursor = $c->find();
while ($cursor->hasNext()) {
  var_dump($cursor->next());
}
?>
--EXPECT--
int(15)
int(0)
