--TEST--
Public Properties should be Read Only
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$d = new MongoDate("1263513600");
$d->sec = 123;
$d->usec = 400;
var_dump($d);

$b = new MongoBinData("binary", 0);
$b->type = 123;
$b->bin = 42;
var_dump($b);


$code = new MongoCode("some code", array("a" => "scope"));
$code->code = "stuff";
$code->scope = new stdclass;
var_dump($code);


$id = new MongoId("51927cff44415ea9024c732f");
$id->{'$id'} = "123";
var_dump($id);


$int32 = new MongoInt32("22");
$int32->value = 42;
var_dump($int32);


$int64 = new MongoInt64("22");
$int64->value = 42;
var_dump($int64);


$regex = new MongoRegex("/asdf/");
$regex->regex = "regex";
$regex->flags = 42;
var_dump($regex);


$ts = new MongoTimestamp(1368554677);
$ts->sec = 42;
$ts->inc = 123;
var_dump($ts);



$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);

$db = $m->selectDb(dbname());
$db->w = 42;
$db->wtimeout = 42;
var_dump($db);

$collection = $db->readonly;
/* FIXME: This two are super difficult to deprecate since GridFS extends Collection */
$collection->w = 42;
$collection->wtimeout = 42;
var_dump($collection);

$gridfs = $db->getGridFS();
$gridfs->chunks = "chunks";
$gridfs->w = 42;
var_dump($gridfs);


?>
===DONE==
<?php exit(0); ?>
--EXPECTF--
%s: The 'sec' property is read-only in %s on line %d

%s: The 'usec' property is read-only in %s on line %d
object(MongoDate)#%d (2) {
  ["sec"]=>
  int(1263513600)
  ["usec"]=>
  int(0)
}

%s: The 'type' property is read-only in %s on line %d

%s: The 'bin' property is read-only in %s on line %d
object(MongoBinData)#%d (2) {
  ["bin"]=>
  string(6) "binary"
  ["type"]=>
  int(0)
}

%s: The 'code' property is read-only in %s on line %d

%s: The 'scope' property is read-only in %s on line %d
object(MongoCode)#%d (2) {
  ["code"]=>
  string(9) "some code"
  ["scope"]=>
  array(1) {
    ["a"]=>
    string(5) "scope"
  }
}

%s: The '$id' property is read-only in %s on line %d
object(MongoId)#%d (1) {
  ["$id"]=>
  string(24) "51927cff44415ea9024c732f"
}

%s: The 'value' property is read-only in %s on line %d
object(MongoInt32)#%d (1) {
  ["value"]=>
  string(2) "22"
}

%s: The 'value' property is read-only in %s on line %d
object(MongoInt64)#%d (1) {
  ["value"]=>
  string(2) "22"
}

%s: The 'regex' property is read-only in %s on line %d

%s: The 'flags' property is read-only in %s on line %d
object(MongoRegex)#%d (2) {
  ["regex"]=>
  string(4) "asdf"
  ["flags"]=>
  string(0) ""
}

%s: The 'sec' property is read-only in %s on line %d

%s: The 'inc' property is read-only in %s on line %d
object(MongoTimestamp)#%d (2) {
  ["sec"]=>
  int(1368554677)
  ["inc"]=>
  int(0)
}

%s: The 'w' property is read-only in %s on line %d

%s: The 'wtimeout' property is read-only in %s on line %d
object(MongoDB)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(MongoCollection)#%d (2) {
  ["w"]=>
  int(42)
  ["wtimeout"]=>
  int(42)
}

%s: The 'chunks' property is read-only in %s on line %d
object(MongoGridFS)#%d (5) {
  ["w"]=>
  int(42)
  ["wtimeout"]=>
  int(10000)
  ["chunks"]=>
  object(MongoCollection)#13 (2) {
    ["w"]=>
    int(1)
    ["wtimeout"]=>
    int(10000)
  }
  ["filesName%s]=>
  string(8) "fs.files"
  ["chunksName%s]=>
  string(9) "fs.chunks"
}
===DONE==
