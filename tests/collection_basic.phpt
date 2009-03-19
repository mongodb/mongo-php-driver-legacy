--TEST--
MongoCollection class - basic collection functionality
--FILE--
<?php
include "Mongo.php";

$m = new Mongo();
$c = $m->selectCollection("phpt", "foo.bar");
echo "$c\n";
echo $c->getName()."\n";
$c->drop();

for($i=0;$i<15;$i++) {
  $c->insert(array("i"=>$i));
}

$c->ensureIndex(array("i"=>1));

$cursor = $c->find()->sort(array("i"=>1));
while ($cursor->hasNext()) {
  $obj = $cursor->next();
  echo $obj['i']." ";
}
echo "\n";

$cursor = $c->find()->sort(array("i"=>-1));
while ($cursor->hasNext()) {
  $obj = $cursor->next();
  echo $obj['i']." ";
}
echo "\n";

$c->drop();
$cursor = $c->find()->sort(array("i"=>1));
while ($cursor->hasNext()) {
  $obj = $cursor->next();
  echo $obj['i']." ";
}
?>
--EXPECT--
phpt.foo.bar
foo.bar
0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 
14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
