--TEST--
Test for PHP-413: Connection strings: unsuccesfull authentication.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$user = username();
$pass = password();
$pass .= "bogus";
$db   = dbname();

try {
    $m = new mongo(sprintf("mongodb://%s:%s@%s:%d/%s", "", $pass, $host, $port, $db));
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}
try {
    $m = new mongo(sprintf("mongodb://%s:%s@%s:%d/%s", $user, "", $host, $port, $db));
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}
try {
    $m = new mongo(sprintf("mongodb://%s:%s@%s:%d/%s", $user, $pass, $host, $port, $db));
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
Failed to connect to: %s:%d: Authentication failed on database '%s' with username '%S': auth fails
Failed to connect to: %s:%d: Authentication failed on database '%s' with username '%S': auth fails
Failed to connect to: %s:%d: Authentication failed on database '%s' with username '%S': auth fails
