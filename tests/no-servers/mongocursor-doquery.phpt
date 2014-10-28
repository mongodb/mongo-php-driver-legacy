--TEST--
MongoCursor::doQuery()
--FILE--
<?php
class myCursor extends MongoCursor
{
	public function doQuery()
	{
		return parent::doQuery();
	}
}
?>
--EXPECTF--
Fatal error: Cannot override final method MongoCursor::doQuery() in %smongocursor-doquery.php on line %d
