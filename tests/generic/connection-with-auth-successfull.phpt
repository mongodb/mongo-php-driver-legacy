--TEST--
Connection strings: succesfull authentication
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
<?php die("skip FIXME: This test executes local binaries..."); ?>
--FILE--
<?php
passthru("mongo tests/addUser.js");
$a = new Mongo("mongodb://testUser:testPass@localhost");
var_dump($a->connected);
echo $a, "\n";
?>
--EXPECTF--
MongoDB shell version: %s
connecting to: test
connecting to: admin
{
	"updatedExisting" : true,
	"n" : 1,
	"connectionId" : %d,
	"err" : null,
	"ok" : 1
}
{
	"_id" : ObjectId("%s"),
	"user" : "testUser",
	"readOnly" : false,
	"pwd" : "03b9b27e0abf1865e2f6fcbd9845dd59"
}
bool(true)
localhost:27017
