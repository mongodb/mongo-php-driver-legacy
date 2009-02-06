<?php

require "src/mongo.php";

$m = new mongo();

function listDBs() {
  global $m;
  $x = $m->list_databases();
  if( $x ) {
    foreach( $x as $k => $v ) {
      echo $v["name"] . "\n";
    }
  }
}

function createColl() {
  global $m;
  $db = $m->select_database( "driver_test_framework" );
  $x = $db->create_collection( "mooo" );
}

function valid() {
  global $m;
  $db = $m->select_database( "driver_test_framework" );
  $collection = $db->select_collection( "mooo" );
  $x = $collection->validate();
  if( $x ) {
    foreach( $x as $k => $v ) {
      echo "$k=$v\n";
    }
  }
  else {
    echo "oops\n";
  }
}

function dropper() {
  global $m;
  $db = $m->select_database( "driver_test_framework" );
  $x = $db->drop_collection( $db->select_collection( "mooo" ) );
  if( $x ) {
    foreach( $x as $k => $v ) {
      echo "$k=$v\n";
    }
  }
  else {
    echo "oops\n";
  }
}

listDBs();
createColl();
valid();
dropper();

$m->close();

?>
