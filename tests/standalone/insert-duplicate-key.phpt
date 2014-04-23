--TEST--
MongoCollection->insert() Duplicate Key Exception
--SKIPIF--
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
    var_dump($e->getCode());
    echo $e->getMessage(), "\n";
}

$tmp = $collection->findOne(array("_id" => $id));
var_dump($tmp == $doc);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
int(11000)
%s:%d:%S E11000 duplicate key error index: %s.dupkey.$_id_  dup key: { : "DuplicateKeyError" }
bool(true)
===DONE===
