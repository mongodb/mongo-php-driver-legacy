--TEST--
MongoCollection::toIndexString
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
if (version_compare( phpversion(), '5.3', '<' )) {
	ini_set('error_reporting', E_ALL & ~E_STRICT);
} else {
	ini_set('error_reporting', E_ALL & ~E_DEPRECATED);
}

class MyCollection extends MongoCollection
{
	static public function toIndexString($a)
	{
		return parent::toIndexString($a);
	}
}

$tests = array(
	'x', 'x.y.z', 'x_y.z',
	array('x' => 1),
	array('x' => -1),
	array('x' => 1, 'y' => -1),
	array('x' => '2dsphere', 'y' => 1),
	array('x' => 'text', 'y' => '2d'),
);

foreach ($tests as $test) {
	var_dump(MyCollection::toIndexString($test));
}
?>
--EXPECT--
string(3) "x_1"
string(7) "x_y_z_1"
string(7) "x_y_z_1"
string(3) "x_1"
string(4) "x_-1"
string(8) "x_1_y_-1"
string(14) "x_2dsphere_y_1"
string(11) "x_text_y_2d"
