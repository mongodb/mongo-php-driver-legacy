--TEST--
MongoAuth class - basic authentication functionality
--FILE--
<?php

include "mongo.php";

$m = new Mongo();
$c = $m->selectDB("phpt")->selectCollection("batch.insert");
$c->drop();

$a = array( array( "x" => "y"), array( "x"=> "z"), array("x"=>"foo"));
$c->batchInsert($a);
var_dump($c->count());

$cursor = $c->find()->sort(array("x" => -1));
while($cursor->hasNext()) {
  $x = $cursor->next();
  var_dump($x['x']);
}

?>
--EXPECT--
float(3)
string(1) "z"
string(1) "y"
string(3) "foo"
