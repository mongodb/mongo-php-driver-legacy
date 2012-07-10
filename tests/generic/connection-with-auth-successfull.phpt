--TEST--
Connection strings: succesfull authentication
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
<?php if (!strstr("AUTH", $_ENV["MONGO_SERVER"])) die("skip Only applicable in authenticated environments") ?>
--FILE--
<?php require_once __DIR__ ."/skipif.inc"; ?>
<?php
$a = mongo(dbname());
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
