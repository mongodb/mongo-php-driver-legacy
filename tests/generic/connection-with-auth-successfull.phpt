--TEST--
Connection strings: succesfull authentication
--SKIPIF--
<?php if (!strstr($_ENV["MONGO_SERVER"], "AUTH")) die("skip Only applicable in authenticated environments" ); ?>
--FILE--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php
$a = mongo(dbname());
var_dump($a->connected);
echo $a, "\n";
?>
--EXPECTF--
bool(true)
%s
