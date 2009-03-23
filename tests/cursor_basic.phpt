--TEST--
MongoCursor class - basic cursor functionality
--FILE--
<?php

include "Mongo.php";

$m = new Mongo();
$c = $m->selectCollection("phpt", "cursor");
$cursor = $c->find();
$cursor->hasNext();
try {
  $cursor->limit(3);
}
catch(MongoException $e) {
  echo "caught exception\n";
}

?>
--EXPECT--
caught exception
