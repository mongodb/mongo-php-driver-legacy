--TEST--
Connection strings: succesfull authentication
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php if (!strstr($_ENV["MONGO_SERVER"], "AUTH")) die("skip Only applicable in authenticated environments" ); ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$a = mongo(dbname());
var_dump($a->connected);
echo $a, "\n";
?>
--EXPECTF--
bool(true)
%s
