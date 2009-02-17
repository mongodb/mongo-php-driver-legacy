<?php

require "mongo.php";

$m = new Mongo();
$db = $m->selectDB( "foo" );

// drop a bunch of times in a row
$c = $db->selectCollection( "d1" );
$c->drop();
$c->drop();
$c->drop();

// basic drop
$c = $db->selectCollection( "d2" );
$c->drop();
$c->insert( array( "x" => 1 ) );
$c->insert( array( "x" => 2 ) );
$c->insert( array( "x" => 3 ) );
$c->insert( array( "x" => 4 ) );
assert( $c->count() == 4 );
$c->drop();
assert( $c->count() == 0 );

// drop idxes
$c = $db->selectCollection( "bar" );
$c->insert( array( "x" => 1 ) );
$c->insert( array( "x" => 2 ) );
$c->insert( array( "x" => 3 ) );
$c->insert( array( "x" => 4 ) );

$c->ensureIndex( "x" );

$c->drop();
assert( $c->count() == 0 );

?>
