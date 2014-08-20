--TEST--
Test for PHP-1036: Segmentation Fault when querying large collection and the working set is not loaded (streams)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php
try {
    $host = MongoShellServer::getStandaloneInfo();
    $mc = new MongoClient($host, array("socketTimeoutMS" => 1));
} catch(Exception $e) {
    die("skip Can't connect to the server fast enough\n");
}
?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

for($i=0; $i<1000; ++$i) {
    $collection->insert(array("bunch" => "of", "documents" => $i));
}
$mc->close();

$mc = new MongoClient($host, array("socketTimeoutMS" => 1));
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
try {
    $retval = $collection->distinct("documents");
    echo "hmh. you have to fast server!\n";
    var_dump(count($retval));
} catch(MongoCursorTimeoutException $e) {
    echo $e->getMessage(), "\n";
} catch(Exception $e) {
    /* Should have thrown cursortimeout exception.. :( */
    var_dump(get_class($e));
    echo $e->getMessage(), "\n";
}
echo "I'm alive!\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
%s:%d: Read timed out after reading %d bytes, waited for 0.%d seconds
I'm alive!
===DONE===
