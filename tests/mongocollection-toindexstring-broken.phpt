--TEST--
MongoCollection::toIndexString (broken)
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
?>
--EXPECTF--
Catchable fatal error: MongoCollection::toIndexString(): The key needs to be either a string or an array in %smongocollection-toindexstring-broken.php on line 6
