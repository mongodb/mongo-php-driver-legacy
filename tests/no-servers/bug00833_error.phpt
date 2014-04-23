--TEST--
Test for PHP-833: Killcursor with wrong hash.
--FILE--
<?php
MongoClient::killCursor( "abacadabra!", 42 );
echo "DONE\n";
?>
--EXPECTF--
Warning: MongoClient::killCursor(): A connection with hash 'abacadabra!' does not exist in %sbug00833_error.php on line %d
DONE
