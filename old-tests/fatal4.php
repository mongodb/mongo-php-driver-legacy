<?php

$m = new Mongo();
$c = $m->foo->bar;
$c->remove();

$c->insert($GLOBALS);

?>
