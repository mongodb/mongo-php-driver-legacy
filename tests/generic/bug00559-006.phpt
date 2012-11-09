--TEST--
Test for PHP-559: The wrong connection is sometimes picked when there are two connections open (two request test).
--DESCRIPTION--
This test only makes sense when run through PHP's built-in webserver,
but I have no idea on how to script that yet. In general, this tests
whether primary nodes are picked up even though the connection string
only includes a secondary node.
--SKIPIF--
skip Manual test
--FILE--
<?php
MongoLog::setModule( MongoLog::ALL ); MongoLog::setLevel( MongoLog::ALL );
set_error_handler( function( $a, $b ) { echo $b, "\n"; } );
echo "\n<pre>CREATING CONNECTION\n";
$a = new Mongo("mongodb://whisky:13001/?replicaset=seta&readPreference=nearest");

echo "\nQUERY\n";
$a->phpunit->test->findOne( array( 'foo' => 'bar' ) );

echo "\nINSERT\n";
$a->phpunit->test->insert( array( 'foo' => 'bar' ) );

echo "</pre>\n";
var_dump( $a->getConnections() );

?>
--EXPECTF--
