--TEST--
Test for PHP-611: Segfault when no candidate servers found.
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$hostname = hostname();
$opts = array(
    "readPreference"     => MongoClient::RP_PRIMARY_PREFERRED,
    "readPreferenceTags" => "dc:no;dc:eu;",
    "replicaSet"         => rsname(),
);
try {
    $m = new MongoClient($hostname, $opts);
} catch(MongoConnectionException $e) {
    echo $e->getMessage(), "\n";
}
echo "I'm alive\n";
?>
--EXPECT--
No candidate servers found
I'm alive
