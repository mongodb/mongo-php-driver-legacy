<?php

$m = new Mongo("mongodb://localhost:27017", array("replicaSet" => true));

$c = $m->foo->bar;

while (true) {
  echo "finding... ";
  try {
    $c->findOne();
  }
  catch (Exception $e) {
    echo $e->getMessage();
  }
  echo "\n";

  sleep(1);
}

?>
