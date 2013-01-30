<?php

if (!extension_loaded('mongo')) {
  dl('mongo.so');
}
$m = new Mongo();
$c = new MongoCursor($m, "bar");
$c2 = clone $c;

?>
