--TEST--
MongoCollection::toIndexString
--FILE--
<?php
class MyCollection extends MongoCollection
{
	static public function toIndexString($a)
	{
		return parent::toIndexString($a);
	}
}
var_dump(MyCollection::toIndexString(null));
var_dump(MyCollection::toIndexString(52));
var_dump(MyCollection::toIndexString('x'));
var_dump(MyCollection::toIndexString('x.y.z'));
var_dump(MyCollection::toIndexString('x_y.z'));
var_dump(MyCollection::toIndexString(array('x' => 1)));
var_dump(MyCollection::toIndexString(array('x' => -1)));
var_dump(MyCollection::toIndexString(array('x' => 1, 'y' => -1)));
?>
--EXPECT--
string(2) "_1"
string(4) "52_1"
string(3) "x_1"
string(7) "x_y_z_1"
string(7) "x_y_z_1"
string(3) "x_1"
string(4) "x_-1"
string(8) "x_1_y_-1"
