--TEST--
Connection strings: unsuccesfull authentication
--FILE--
<?php
passthru("mongo tests/addUser.js");
try {
	$a = new Mongo("mongodb://testUser:testP@localhost");
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
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
Couldn't authenticate with database admin: username [testUser], password [testP]
