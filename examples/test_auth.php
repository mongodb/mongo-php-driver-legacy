<?php

include "src/php/mongo.php";

$m = new mongo();
$a = $m->get_auth( "admin", "kristina", "fred" );
echo $a . "\n";


?>
