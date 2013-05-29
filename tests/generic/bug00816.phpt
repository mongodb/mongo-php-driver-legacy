--TEST--
Test for bug PHP-816: MongoCursor doesn't validate the namespace
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
class MyMongoClient extends MongoClient {
}
class MyDB extends MongoDB {
    public function __construct() {}
}
 
$db = new MyDB;

$nss = array(
	'test', '.test', 'test.', '', '.', 'xx', 'xxx', 'a.b', 'db.foo', 'foo.file.fs',
);

foreach ($nss as $ns) {
	echo "NS: ", $ns, ": ";
	try {
		$c = new MongoCursor(new MyMongoClient($dsn), $ns);
		$c->hasNext();
		echo "OK\n";
	} catch (MongoException $e) {
		echo "FAIL\n";
		var_dump($e->getCode());
		var_dump($e->getMessage());
	}
	echo "\n";
}
?>
--EXPECT--
NS: test: FAIL
int(21)
string(40) "An invalid 'ns' argument is given (test)"

NS: .test: FAIL
int(21)
string(41) "An invalid 'ns' argument is given (.test)"

NS: test.: FAIL
int(21)
string(41) "An invalid 'ns' argument is given (test.)"

NS: : FAIL
int(21)
string(36) "An invalid 'ns' argument is given ()"

NS: .: FAIL
int(21)
string(37) "An invalid 'ns' argument is given (.)"

NS: xx: FAIL
int(21)
string(38) "An invalid 'ns' argument is given (xx)"

NS: xxx: FAIL
int(21)
string(39) "An invalid 'ns' argument is given (xxx)"

NS: a.b: OK

NS: db.foo: OK

NS: foo.file.fs: OK
