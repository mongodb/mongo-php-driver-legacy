--TEST--
Database: valid name checks
--FILE--
<?php
$a = new Mongo("localhost");
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
