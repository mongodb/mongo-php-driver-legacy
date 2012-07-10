--TEST--
Database: valid name checks
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$a = mongo();
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
\: MongoDB::__construct(): invalid name \
$: MongoDB::__construct(): invalid name $
/: MongoDB::__construct(): invalid name /
foo.bar: MongoDB::__construct(): invalid name foo.bar
