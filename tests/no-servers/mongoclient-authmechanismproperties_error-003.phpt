--TEST--
MongoClient option parsing error: authMechanismProperties unknown property
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php include 'tests/utils/server.inc'; ?>
<?php

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
set_error_handler(function($errno, $errstr) { echo $errstr, "\n"; });

echo "Testing unknown property alone\n";

try {
    new MongoClient(null, array(
        'connect' => false,
        'authMechanism' => 'GSSAPI',
        'authMechanismProperties' => 'FOO:bar',
    ));
} catch (MongoConnectionException $e) {
    var_dump($e->getCode());
    var_dump($e->getMessage());
}

echo "\nTesting unknown property after valid property\n";

try {
    new MongoClient(null, array(
        'connect' => false,
        'authMechanism' => 'GSSAPI',
        'authMechanismProperties' => 'SERVICE_NAME:foo,FOO:bar',
    ));
} catch (MongoConnectionException $e) {
    var_dump($e->getCode());
    var_dump($e->getMessage());
}

?>
===DONE===
--EXPECT--
Testing unknown property alone
PARSE   INFO: Parsing localhost:27017
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'authMechanism': 'GSSAPI'
PARSE   INFO: - Found option 'authMechanismProperties': 'FOO:bar'
PARSE   WARN: - Found unknown auth mechanism property 'FOO' with value 'bar'
int(22)
string(62) "- Found unknown auth mechanism property 'FOO' with value 'bar'"

Testing unknown property after valid property
PARSE   INFO: Parsing localhost:27017
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'authMechanism': 'GSSAPI'
PARSE   INFO: - Found option 'authMechanismProperties': 'SERVICE_NAME:foo,FOO:bar'
PARSE   INFO: - Found auth mechanism property 'SERVICE_NAME': 'foo'
PARSE   WARN: - Found unknown auth mechanism property 'FOO' with value 'bar'
int(22)
string(62) "- Found unknown auth mechanism property 'FOO' with value 'bar'"
===DONE===
