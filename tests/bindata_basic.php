<?php

include "mongo.php";

$m=new Mongo();

$bin = new MongoBinData("abcdefg");
$c=$m->selectCollection("phpt", "bindata");
$c->drop();
$c->insert(array("bin"=>$bin));

$obj = $c->findOne();
var_dump($obj);
var_dump($obj["bin"]);

?>
