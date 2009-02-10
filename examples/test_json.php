<?php

include "src/php/mongo.php";

$a = array(  "name" => "MongoDB",
             "type" => "database",
             "count" => 1,
             "info" => array( "x" => 203,
                              "y" => 102
                              )
             );

echo mongo_util::to_json( $a ) . "\n";

?>
