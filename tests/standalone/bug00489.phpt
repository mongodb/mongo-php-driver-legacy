--TEST--
Test for PHP-489: is_master() crashes for standalone servers
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
try {
    $m = new Mongo(hostname(), array("replicaSet" => true));
} catch(MongoConnectionException $e) {
    var_dump($e->getMessage());
}
echo "I'm alive!\n";
?>
==DONE==
--EXPECT--
string(26) "No candidate servers found"
I'm alive!
==DONE==
