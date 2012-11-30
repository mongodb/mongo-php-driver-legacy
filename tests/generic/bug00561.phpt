--TEST--
Test for PHP-561: Handle empty database name better
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";

MongoLog::setModule( MongoLog::PARSE );
MongoLog::setLevel( MongoLog::INFO );
set_error_handler( 'foo' ); function foo($a, $b, $c) { echo $b, "\n"; }

new MongoClient("mongodb://localhost", array( 'connect' => false ));
new MongoClient("mongodb://localhost/", array( 'connect' => false ));
new MongoClient("mongodb://localhost/?readPreference=PRIMARY", array( 'connect' => false ));
?>
--EXPECT--
PARSE   INFO: Parsing mongodb://localhost
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: Parsing mongodb://localhost/
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: Parsing mongodb://localhost/
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: Parsing mongodb://localhost/?readPreference=PRIMARY
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'readPreference': 'PRIMARY'
