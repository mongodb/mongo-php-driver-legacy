--TEST--
MongoCollection::toIndexString
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
class MyCollection extends MongoCollection
{
	static public function toIndexString($a)
	{
		return parent::toIndexString($a);
	}
}
var_dump(MyCollection::toIndexString('x'));
var_dump(MyCollection::toIndexString('x.y.z'));
var_dump(MyCollection::toIndexString('x_y.z'));
var_dump(MyCollection::toIndexString(array('x' => 1)));
var_dump(MyCollection::toIndexString(array('x' => -1)));
var_dump(MyCollection::toIndexString(array('x' => 1, 'y' => -1)));
?>
--EXPECTF--
Deprecated: Function MongoCollection::toIndexString() is deprecated in %smongocollection-toindexstring.php on line %d
string(3) "x_1"

Deprecated: Function MongoCollection::toIndexString() is deprecated in %smongocollection-toindexstring.php on line %d
string(7) "x_y_z_1"

Deprecated: Function MongoCollection::toIndexString() is deprecated in %smongocollection-toindexstring.php on line %d
string(7) "x_y_z_1"

Deprecated: Function MongoCollection::toIndexString() is deprecated in %smongocollection-toindexstring.php on line %d
string(3) "x_1"

Deprecated: Function MongoCollection::toIndexString() is deprecated in %smongocollection-toindexstring.php on line %d
string(4) "x_-1"

Deprecated: Function MongoCollection::toIndexString() is deprecated in %smongocollection-toindexstring.php on line %d
string(8) "x_1_y_-1"
