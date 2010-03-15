<?php

$m = new Mongo("localhost:27017", array("persist" => "x"));

for ($i=0; $i<100; $i++) {
  echo "connecting...\n";
  $m = new Mongo("localhost:27017", array("persist" => "x"));
  sleep(1);
}

?>
