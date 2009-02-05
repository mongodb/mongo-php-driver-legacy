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

listDBs();
//createColl();

?>
