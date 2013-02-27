--TEST--
Test for PHP-413: Connection strings: unsuccesfull authentication.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$host = hostname();
$port = standalone_port();
$user = "A";
$pass = "wrong password";
$db   = dbname();

try {
    $m = new mongo(sprintf("mongodb://%s:%s@%s:%d/%s", "", $pass, $host, $port, $db));
    var_dump("Failed");
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}
try {
    $m = new mongo(sprintf("mongodb://%s:%s@%s:%d/%s", $user, "", $host, $port, $db));
    var_dump("Failed");
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}
try {
    $m = new mongo(sprintf("mongodb://%s:%s@%s:%d/%s", $user . "bogus", $pass, $host, $port, $db));
    var_dump("Failed");
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
Failed to connect to: %s:%d: Authentication failed on database '%s' with username '%S': auth fails
Failed to connect to: %s:%d: Authentication failed on database '%s' with username '%S': auth%s
Failed to connect to: %s:%d: Authentication failed on database '%s' with username '%S': auth%s
