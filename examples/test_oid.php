<?php

//dl("libmongo.so"); 
include "src/mongo.php";

$o=new mongo_id(); 
$conn=new mongo(); 
//$z = $conn->select_database("foo")->select_collection("bar")->insert( array( "x" => new mongo_id() ) );
$z = $conn->select_database("foo")->select_collection("bar")->find();
while( $z->has_next() ){
  $x = $z->next();
  echo "next:\n";
  foreach( $x as $k=>$v ) {
    echo "$k=$v\n";
  }
}

?>
