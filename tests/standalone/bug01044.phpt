--TEST--
Test for PHP-906: Segmentation Fault - in mongo_deregister_callback_from_connection
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!MONGO_SUPPORTS_AUTH_MECHANISM_PLAIN) { exit("skip needs mechanism=plain enabled"); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
try {
    $mc = new MongoClient($host . '/$external?authMechanism=PLAIN');
} catch(Exception $e) {
    /* Doesn't matter if the server actually doesn't support the mechanism, as long as the client did */
}

echo "I should still be alive!\n";
?>
--EXPECT--
I should still be alive!

