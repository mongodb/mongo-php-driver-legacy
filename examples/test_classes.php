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

function insert() {
  global $m;
  $coll = $m->select_database( "driver_test_framework" )->select_collection( "foo" );
  $arr = array( "this" => "that", "foo"=>"bar2" );
  $obj = $coll->insert( $arr );
  if( $obj ) {
    foreach( $obj as $k => $v ) {
      echo "$k=$v\n";
    }
  }
}

function find() {
  global $m;
  $coll = $m->select_database( "driver_test_framework" )->select_collection( "foo" );
  $cursor = $coll->find()->limit( 2 )->skip( 1 );
  while( $cursor->has_next() ) {
    echo "next obj:\n";
    $obj = $cursor->next();
    foreach( $obj as $k => $v ) {
      echo "\t$k=$v\n";
    }
  }
}

function auth() {
  global $m;
  $db = $m->select_database( "admin" );
  $x = $db->get_auth( "kristina", "fred" );
  echo "auth? $x\n";
  if( $x ) {
    $x->set_logging( 2 );
    $x->logout();
  }
}

function update() {
  global $m;
  $coll = $m->select_database( "driver_test_framework" )->select_collection("foo");
  $u = $coll->update( array( "y" => 0 ), array( "y" => 6, "x" => 2 ), true );
  echo "u: $u\n";
  find();
}

function rm() {
  global $m;
  $coll = $m->select_database( "driver_test_framework" )->select_collection("foo");
  $u = $coll->remove( array( "x" => 4 ), true );
  echo "u: $u\n";
  find();
}

function idxs() {
  global $m;
  $coll = $m->select_database( "driver_test_framework" )->select_collection("foo");
  $coll->ensure_index( "x" );
  $coll->ensure_index( array( "x" => 1, "y"=>1 ) );
  $coll->delete_index( "x" );
}

//listDBs();
//createColl();
//valid();
//dropper();
//profiling();
//insert();
//find();
//auth();
//update();
//rm();
idxs();

$m->close();

?>
