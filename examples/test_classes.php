<?php

require "mongo.php";

$m = new Mongo();

function createColl() {
  global $m;
  $db = $m->selectDB( "driver_test_framework" );
  $x = $db->createCollection( "mooo" );
}

function valid() {
  global $m;
  $db = $m->selectDB( "driver_test_framework" );
  $collection = $db->selectCollection( "mooo" );
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
  $db = $m->selectDB( "driver_test_framework" );
  $x = $db->dropCollection( $db->selectCollection( "mooo" ) );
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
  $db = $m->selectDB( "driver_test_framework" );
  $x = $db->getProfilingLevel();
  echo "profiling level: $x\n";

  $x = $db->setProfilingLevel( 1 );
  echo "profiling level was: $x\n";

  $x = $db->setProfilingLevel( 0 );
  echo "profiling level was: $x\n";

  $x = $db->setProfilingLevel( "foo" );
  echo "profiling level was: $x\n";
}

function insert() {
  global $m;
  $coll = $m->selectDB( "driver_test_framework" )->selectCollection( "foo" );
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
  $coll = $m->selectDB( "driver_test_framework" )->selectCollection( "foo" );
  $cursor = $coll->find()->limit( 2 )->skip( 1 )->sort( array( "x" => 1 ) );
  while( $cursor->hasNext() ) {
    echo "next obj:\n";
    $obj = $cursor->next();
    foreach( $obj as $k => $v ) {
      echo "\t$k=$v\n";
    }
  }
}

function auth() {
  global $m;
  $db = $m->selectDB( "admin" );
  $x = $db->getAuth( "kristina", "fred" );
  echo "auth? $x\n";
  if( $x ) {
    $x->setLogging( 2 );

    $list = $x->listDBs();
    if( $list ) {
      foreach( $list as $k => $v ) {
        echo $v["name"] . "\n";
      }
    }

    $x->logout();
  }
}

function update() {
  global $m;
  $coll = $m->selectDB( "driver_test_framework" )->selectCollection("foo");
  $u = $coll->update( array( "y" => 0 ), array( "y" => 6, "x" => 2 ), true );
  echo "u: $u\n";
  find();
}

function rm() {
  global $m;
  $coll = $m->selectDB( "driver_test_framework" )->selectCollection("foo");
  $u = $coll->remove( array( "x" => 4 ), true );
  echo "u: $u\n";
  find();
}

function idxs() {
  global $m;
  $coll = $m->selectDB( "driver_test_framework" )->selectCollection("foo");
  $coll->ensureIndex( "x" );
  $coll->ensureIndex( array( "x" => 1, "y"=>1 ) );
  $coll->deleteIndex( "x" );
}

createColl();
valid();
dropper();
profiling();
insert();
find();
auth();
update();
rm();
idxs();

$m->close();

?>
