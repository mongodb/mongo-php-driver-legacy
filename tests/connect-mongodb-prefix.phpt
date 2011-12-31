--TEST--
Connection strings: Prefixed with mongodb://
--FILE--
<?php
function errorHandler($nr, $str)
{
	throw new Exception($str, $nr);
	return true;
}
set_error_handler('errorHandler');

try {
	$a = new Mongo("mongodb://localhost", false);
} catch( Exception $e ) {
	echo $e->getMessage(), "\n";
}

$a = new Mongo("mongodb://localhost");
var_dump($a->connected);

try {
	$b = new Mongo("mongodb://localhost:27017", false);
} catch( Exception $e ) {
	echo $e->getMessage(), "\n";
}

$b = new Mongo("mongodb://localhost:27017");
var_dump($b->connected);
--EXPECT--
Argument 2 passed to Mongo::__construct() must be an array, boolean given
bool(true)
Argument 2 passed to Mongo::__construct() must be an array, boolean given
bool(true)
