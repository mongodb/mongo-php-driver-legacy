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

function profiling() {
  global $m;
  $db = $m->select_database( "driver_test_framework" );
  $x = $db->get_profiling_level();
  echo "profiling level: $x\n";

  $x = $db->set_profiling_level( 1 );
  echo "profiling level was: $x\n";

  $x = $db->set_profiling_level( 0 );
  echo "profiling level was: $x\n";

  $x = $db->set_profiling_level( "foo" );
  echo "profiling level was: $x\n";
}

/*
listDBs();
createColl();
valid();
dropper();
*/
profiling();

$m->close();

?>
