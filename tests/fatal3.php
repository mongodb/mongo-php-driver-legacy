<?php

$mongo = new Mongo();

$cursor = $mongo->foo->bar->find();

foreach ($cursor as $value) {
  $value->foo();
}

?>
