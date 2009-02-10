<?php

include "src/php/mongo.php";

$m = new Mongo();
$c = $m->select_db( "driver_test_framework" )->select_collection( "foo" );

echo "count: " . $c->count() . "\n";
$cursor = $c->find();
echo "count: " . $cursor->count() . "\n";

?>
