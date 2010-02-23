<?php

$mongo = new Mongo("mongodb://kristina:foo@localhost");
$col = $mongo->foo->bar;

$count = 0;
while ($count < 10) {
  echo "inserting $count...\n";
  try {
    $cursor = $col->insert(array("name" => 1), array("safe" => true));
  }
  catch(MongoException $e) {
    echo "connection exception!\n";
  }
  sleep(3);
  $count++;
}


?>
