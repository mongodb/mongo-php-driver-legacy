--TEST--
Database: valid name checks
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$a = mongo_standalone();
$names = array("\\", "\$", "/", "foo.bar");
foreach ($names as $name) {
	try {
		$d = new MongoDB($a, $name);
	} catch (Exception $e) {
		echo $name, ": ", $e->getMessage(), "\n";
	}
}
?>
--EXPECT--
\: invalid database name '\'
/: invalid database name '/'
foo.bar: invalid database name 'foo.bar'
