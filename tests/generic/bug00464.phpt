--TEST--
Test for PHP-464: Mongo->connected always false
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$port = port();
if ($port != "27017") {
    die("skip this test attempts to connect to the standard port");
}
?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$hostname = hostname();
$port     = port();

$a = new Mongo($hostname, array( 'connect' => false ));
var_dump($a->connected);
$a->connect();
var_dump($a->connected);
?>
--EXPECTF--
Deprecated: Mongo::__construct(): The 'connect' option is deprecated and will be removed in the future in %sbug00464.php on line %d
bool(false)
bool(true)
