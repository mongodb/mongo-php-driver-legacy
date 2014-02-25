--TEST--
MongoCollection->insert() Duplicate Key Exception
--SKIPIF--
<?php $needs = "2.5.4"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";


$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), "dupkey");
$collection->drop();

$id = "DuplicateKeyError";

$doc = array(
    "example" => "document",
    "with"    => "some",
    "fields"  => "in it",
    "and"     => array(
        "nested", "documents",
    ),
    "_id"     => $id,
);

$opts = array(
    "w"        => 1,
);
$ret = $collection->insert($doc, $opts);

try {
    $ret = $collection->insert($doc, $opts);
    echo "FAILED\nThat should have raised duplicate key exception!\n";
} catch(MongoDuplicateKeyException $e) {
    echo $e->getMessage(), "\n";
}

$tmp = $collection->findOne(array("_id" => $id));
var_dump($tmp == $doc);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
%s:%d: insertDocument :: caused by :: 11000 E11000 duplicate key error index: test.dupkey.$_id_  dup key: { : "DuplicateKeyError" }
bool(true)
===DONE===
