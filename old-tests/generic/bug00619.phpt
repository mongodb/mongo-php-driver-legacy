--TEST--
Test for PHP-619: Connection failure
--SKIPIF--
<?php require_once dirname(__FILE__). "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = new MongoClient(hostname(), array("db" => dbname()));
?>
I'm Alive
==DONE==
--EXPECTF--
I'm Alive
==DONE==
