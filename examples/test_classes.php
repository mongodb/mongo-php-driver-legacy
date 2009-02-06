<?php

require "src/mongo.php";

function listDBs() {
  $m = new Mongo();
  $m->setDatabase( "admin" );
  $x=$m->listDatabases();
  if( $x ) {
    foreach( $x as $k => $v ) {
      echo $v["name"] . "\n";
    }
  }
  $m->close();
}

function createColl() {
  $m = new Mongo();
  $m->setDatabase( "driver_test_framework" );
  $x=$m->createCollection( "mooo" );
  $m->close();
}

function valid() {
  $m = new Mongo();
  $m->setDatabase( "driver_test_framework" );
  $collection = $m->getCollection( "mooo" );
  $x = $collection->validate();
  if( $x ) {
    foreach( $x as $k => $v ) {
      echo "$k=$v\n";
    }
  }
  else {
    echo "oops\n";
  }
  $m->close();
}

//listDBs();
//createColl();
valid();

?>
