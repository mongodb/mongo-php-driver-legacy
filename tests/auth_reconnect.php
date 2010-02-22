<?php

$mongo = new Mongo("mongodb://kristina:foobar@localhost");
$col = $mongo->foo->bar;

while (true) {
  echo "inserting...\n";
  try {
    $cursor = $col->insert(array("name" => 1), array("safe" => true));
  }
  catch(MongoException $e) {
    echo "connection exception!\n";
  }
  sleep(3);
}


?>
