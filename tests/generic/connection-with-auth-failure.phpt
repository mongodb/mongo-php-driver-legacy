--TEST--
Connection strings: unsuccesfull authentication
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
try {
    $host = hostname();
    $port = port();
    $user = username();
    $pass = password();
    $pass .= "bogus";
    $db   = dbname();

    $m = new mongo(sprintf("mongodb://%s:%s@%s:%d/%s", $user, $pass, $host, $port, $db));
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
Couldn't connect to '%s:%d': Authentication failed on database '%s' with username '%S': auth fails
