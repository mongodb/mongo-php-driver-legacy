<?php

$m = new Mongo("mongodb://localhost:27017,localhost:27018");

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
