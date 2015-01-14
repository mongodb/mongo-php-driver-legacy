--TEST--
MongoClient parses authMechanismProperties option
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php include 'tests/utils/server.inc'; ?>
<?php

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
set_error_handler(function($errno, $errstr) { echo $errstr, "\n"; });

echo "Testing with URI parameters\n";

new MongoClient('mongodb://localhost:27017/?authMechanism=GSSAPI&authMechanismProperties=SERVICE_NAME:foo', array(
    'connect' => false,
));

echo "\nTesting with options array\n";

new MongoClient(null, array(
    'connect' => false,
    'authMechanism' => 'GSSAPI',
    'authMechanismProperties' => 'SERVICE_NAME:foo',
));

?>
===DONE===
--EXPECT--
Testing with URI parameters
PARSE   INFO: Parsing mongodb://localhost:27017/?authMechanism=GSSAPI&authMechanismProperties=SERVICE_NAME:foo
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'authMechanism': 'GSSAPI'
PARSE   INFO: - Found option 'authMechanismProperties': 'SERVICE_NAME:foo'
PARSE   INFO: - Found auth mechanism property 'SERVICE_NAME': 'foo'

Testing with options array
PARSE   INFO: Parsing localhost:27017
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'authMechanism': 'GSSAPI'
PARSE   INFO: - Found option 'authMechanismProperties': 'SERVICE_NAME:foo'
PARSE   INFO: - Found auth mechanism property 'SERVICE_NAME': 'foo'
===DONE===
