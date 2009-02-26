<?php

include "mongo.php";

$m = new Mongo();
$c = $m->selectDB("phpt")->selectCollection("batch.insert");
$a = array( array( "x" => "y"), array( "x"=> "z"), array("x"=>"foo"));
$c->batchInsert($a);

?>
