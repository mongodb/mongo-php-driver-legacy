<?php

dl("libmongo.dylib");

$array = array( "x" => 1, "y"=>2, "foo"=>"bar" );

echo "connecting...\n";
if( !($db = mongo_connect( "127.0.0.1" ) ) ) {
  echo "couldn't connect";
  return;
}
echo "inserting...\n";
mongo_insert( $db, "test.itest", $array );
echo "closing...\n";
mongo_close( $db );

?>
