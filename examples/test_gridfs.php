<?php

include "src/php/mongo.php";

$m = new Mongo();
$grid = $m->selectDB( "driver_test_framework" )->getGridfs();
echo "storing /home/k/hair\n";
$id = $grid->storeFile( "/home/k/hair" );
echo "id: $id\n";
echo "getting /home/k/hair\n";
$file = $grid->findFile( array( "filename" => "/home/k/hair" ));
if( $file->exists() ) {
  echo "exists.\n";
  echo "size: " . $file->getSize() . "\n";
  echo "name: " . $file->getFilename() . "\n";
}
else {
  echo "failed\n";
}
$m->close();

?>
