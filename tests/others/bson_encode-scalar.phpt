--TEST--
Scalar input for bson_encode()
--CREDITS--
Stefan Koopmanschap <stefan.koopmanschap@gmail.com>
# PFZ.nl/AmsterdamPHP TestFest 2012-06-23
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
$output = bson_encode('test');
var_dump(bin2hex($output));

$output = bson_encode(1);
var_dump(bin2hex($output));

$output = bson_encode(true);
var_dump(bin2hex($output));

$output = bson_encode(7E-10);
var_dump(bin2hex($output));
?>
--EXPECTF--
string(8) "74657374"
string(8) "01000000"
string(2) "01"
string(16) "03c69cde430d083e"
