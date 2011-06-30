<?php

$m = new Mongo("mongodb://localhost:27017", array("replicaSet" => true));

$c = $m->foo->bar;

while (true) {
  echo "finding... ";
  $cursor = $c->find();

  try {
      $cursor->getNext();
  }
  catch (Exception $e) {
      echo $e->getMessage()." ";
  }
  $info1 = $cursor->info();
  if ($info1['server']) {
      echo $info1['server']." ";
  }
  echo "$m\n";

  try {
      echo "reading... ";
      $cursor = $c->find()->slaveOkay();
      $cursor->next();
      $info2 = $cursor->info();
      echo $info2['server'];
  }
  catch (Exception $e) {
      echo $e->getMessage()." ";
  }
  echo "\n";

  //  var_dump($m->getHosts());

  sleep(1);
}

?>
