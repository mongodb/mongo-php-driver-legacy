--TEST--
Connection strings: unsuccesfull authentication
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php require_once __DIR__ ."/skipif.inc"; ?>
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
Couldn't authenticate with database %s: username [%S]
Couldn't authenticate with database %s: username [%S]
Couldn't authenticate with database %s: username [%S]
