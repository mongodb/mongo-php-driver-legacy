--TEST--
Test for PHP-685: wtimeout option is not supported per-query
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc" ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = new_mongo();

try {
    /* Noway you can replicate to 2 server so fast :D */
    $m->selectDb(dbname())->test->insert(array("random" => "data"), array("wtimeout" => 1, "w" => 2));
} catch(MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--
string(%s) "%s:%d: timeout"
int(4)
