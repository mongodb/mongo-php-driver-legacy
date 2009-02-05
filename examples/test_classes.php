<?php

require "src/mongo.php";

function listDBs() {
  $m = new Mongo();
  $m->setDatabase( "admin" );
  $x=$m->listDatabases();
  if( $x && $x[ "ok" ] == 1 ) {
    $arr = $x[ "databases" ];
    foreach( $arr as $k => $v ) {
      foreach( $arr[ $k ] as $k2=>$v2 ) {
        echo "$k2=$v2\n";
      }
    }
  }
  $m->close();
}

listDBs();

?>
