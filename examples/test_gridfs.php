<?php

include "src/php/mongo.php";

$m = new mongo();
$grid = $m->select_db( "driver_test_framework" )->get_gridfs();
echo "storing /home/k/hair\n";
$id = $grid->store_file( "/home/k/hair" );
echo "id: $id\n";
echo "getting /home/k/hair\n";
$file = $grid->find_file( array( "filename" => "/home/k/hair" ));
if( $file->exists() ) {
  echo "exists.\n";
  echo "size: " . $file->get_size() . "\n";
  echo "name: " . $file->get_filename() . "\n";
}
else {
  echo "failed\n";
}
$m->close();

?>
