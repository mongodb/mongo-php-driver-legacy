<?php

dl('mongo.so');
$m = new Mongo();
$c = new MongoCursor($m->connection, "bar");
$c2 = clone $c;

?>
