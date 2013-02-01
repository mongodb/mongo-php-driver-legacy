--TEST--
Test for PHP-631: One replica set, but two different db/user/passwords
--SKIPIF--
<?php exit("skip Manual test - needs two users on two different databases"); ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";


function queryMongoDB($connstr, $dbname, $collectionname, $fieldname)
{
    $m = new MongoClient($connstr, array('replicaSet' => true)); #just specify it as true instead of actual replica set. Either way the bug is reproduced.
    $db = $m->selectDB($dbname);
    $collection = $db->selectCollection($collectionname);
    $cursor = $collection->find();
    foreach ($cursor as $document) {
    }
}

#MongoLog::setLevel(MongoLog::ALL); // all log levels
#MongoLog::setModule(MongoLog::ALL); // all parts of the driver

queryMongoDB("mongodb://foo:foopassword@primaryauth,secondaryauth/foo", "foo", "foocollection", "fieldinfoocollection");
#Step 2: connect and query to bar db: This would fail randomly with message
queryMongoDB("mongodb://bar:barpassword@primaryauth,secondaryauth/bar", "bar", "barcollection", "fieldinbarcollection.");
?>
--EXPECTF--
