<?php

if (!extension_loaded('mongo')) {
  dl('mongo.so');
}
$id = new MongoId();
$id2 = clone $id;

?>
