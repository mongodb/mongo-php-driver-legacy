--TEST--
MongoCollection class - testing weird input
--FILE--
<?php

include "Mongo.php";

$m = new Mongo();
$c = $m->selectDB("phpt")->selectCollection("insert.weird");

$c->insert(NULL);
$c->insert(array(NULL));
$c->insert(array(NULL=>"1"));

$cursor = $c->find();
while($cursor->hasNext()) {
  var_dump($cursor->next());
}
?>
