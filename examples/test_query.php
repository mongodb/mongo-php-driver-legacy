<?php

dl("libmongo.so");

echo "connecting...\n";
if( !($db = mongo_connect( "127.0.0.1" ) ) ) {
  echo "couldn't connect";
  return;
}
echo "querying...\n";
$cursor = mongo_query( $db, "driver_test_framework.test", array( "a" => 2 ) );
while( mongo_has_next( $cursor ) ){
  echo "cursor: " . mongo_next( $cursor ) . "\n";
}

?>
