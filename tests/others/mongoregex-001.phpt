--TEST--
MongoRegex constructor
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$regex = new MongoRegex('//');
var_dump($regex->regex);
var_dump($regex->flags);

$regex = new MongoRegex('/\w+/im');
var_dump($regex->regex);
var_dump($regex->flags);

$regex = new MongoRegex('/foo[bar]{3}/i');
var_dump($regex->regex);
var_dump($regex->flags);
?>
--EXPECT--
string(0) ""
string(0) ""
string(3) "\w+"
string(2) "im"
string(11) "foo[bar]{3}"
string(1) "i"
