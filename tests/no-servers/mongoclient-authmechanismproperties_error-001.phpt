--TEST--
MongoClient option parsing error: authMechanismProperties missing colon
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php include 'tests/utils/server.inc'; ?>
<?php

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
set_error_handler(function($errno, $errstr) { echo $errstr, "\n"; });

try {
    new MongoClient(null, array(
        'connect' => false,
        'authMechanism' => 'GSSAPI',
        'authMechanismProperties' => 'SERVICE_NAME',
    ));
} catch (MongoConnectionException $e) {
    var_dump($e->getCode());
    var_dump($e->getMessage());
}

?>
===DONE===
--EXPECT--
PARSE   INFO: Parsing localhost:27017
PARSE   INFO: - Found node: localhost:27017
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'authMechanism': 'GSSAPI'
PARSE   INFO: - Found option 'authMechanismProperties': 'SERVICE_NAME'
int(23)
string(86) "Error while trying to parse auth mechanism properties: No separator for 'SERVICE_NAME'"
===DONE===
