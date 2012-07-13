--TEST--
MongoRegex constructor with unsupported or redundant flags
--FILE--
<?php
$regex = new MongoRegex('//nope');
var_dump($regex->regex);
var_dump($regex->flags);

$regex = new MongoRegex('//iii');
var_dump($regex->regex);
var_dump($regex->flags);

$regex = new MongoRegex('//nopenope');
var_dump($regex->regex);
var_dump($regex->flags);
?>
--EXPECT--
string(0) ""
string(4) "nope"
string(0) ""
string(3) "iii"
string(0) ""
string(8) "nopenope"
