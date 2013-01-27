--TEST--
MongoBinData construction with default type
--DESCRIPTION--
This test expects an E_DEPRECATED notice because the default type will change in
version 2.0 of the extension (see: https://jira.mongodb.org/browse/PHP-407).
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--INI--
display_errors=1
--FILE--
<?php
error_reporting(E_ALL);

$numNotices = 0;

function handleNotice($errno, $errstr) {
		global $numNotices;
		++$numNotices;
}

if (!defined("E_DEPRECATED")) {
		define("E_DEPRECATED", E_STRICT);
}
set_error_handler('handleNotice', E_DEPRECATED);

$bin = new MongoBinData('abcdefg');
var_dump($bin->bin);
var_dump($bin->type);
var_dump(1 === $numNotices);
--EXPECT--
string(7) "abcdefg"
int(2)
bool(true)
