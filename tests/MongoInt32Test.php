<?php
require_once 'PHPUnit/Framework.php';

class MongoInt32Test extends PHPUnit_Framework_TestCase
{
	function setup()
	{
		if (PHP_INT_SIZE != 4) {
			$this->markTestSkipped("Only for 32 bit platforms");
		}
		ini_set('mongo.native_long', 0);
		ini_set('mongo.long_as_object', 0);

		$this->object = $this->sharedFixture->selectCollection("phpunit", "ints");
		$this->object->drop();
	}

	function testNormalInsert32()
	{
		$c = $this->object;
		$c->insert(array('int32' => 1234567890));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int32']);
	}

	function testNormalInsert64()
	{
		$c = $this->object;
		$c->insert(array('int64' => 12345678901234567890));
		$x = $c->findOne();
		$this->assertSame(12345678901234567890, $x['int64']);
	}

	function testNormalInsertNativeLong()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int64' => 12345678901234567890));
		$x = $c->findOne();
		$this->assertSame(12345678901234567890, $x['int64']);
	}

	function testNormalLongAsObject()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int64' => 123456789012345));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame(123456789012345, $x['int64']);
	}

	function testNativeLongAsObject()
	{
		ini_set('mongo.native_long', 1);
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int64' => 123456789012345));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame(123456789012345, $x['int64']);
	}

	/** objects with int ctor */
	function testInsertIntMongo32()
	{
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32(1234567890)));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int32']);
	}

	function testInsertIntMongo32B()
	{
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32(1234567890123)));
		$x = $c->findOne();
		$this->assertSame(PHP_INT_MAX, $x['int32']);
	}

	function testInsertIntMongo32NativeLong()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32(1234567890)));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int32']);
	}

	function testInsertIntMongo32NativeLongB()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32(1234567890123)));
		$x = $c->findOne();
		$this->assertSame(PHP_INT_MAX, $x['int32']);
	}

	function testInsertIntMongo32LongAsObject()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32(1234567890)));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int32']);
	}

	function testInsertIntMongo32LongAsObjectB()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32(1234567890123)));
		$x = $c->findOne();
		$this->assertSame(PHP_INT_MAX, $x['int32']);
	}

	function testInsertIntMongo64()
	{
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(1234567890)));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame(1234567890.0, $x['int64']);
	}

	function testInsertIntMongo64B()
	{
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(123456789012345)));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame(1.0, $x['int64']);
	}

	/**
	 * @expectedException MongoCursorException
	 */
	function testInsertIntMongo64NativeLong()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(1234567890)));
		$x = $c->findOne();
	}

	/**
	 * @expectedException MongoCursorException
	 */
	function testInsertIntMongo64NativeLongB()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(123456789012345)));
		$x = $c->findOne();
	}

	function testInsertIntMongo64LongAsObject()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(1234567890)));
		$x = $c->findOne();
		$this->assertEquals(new MongoInt64('1234567890'), $x['int64']);
	}

	function testInsertIntMongo64LongAsObjectB()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(1234567890123)));
		$x = $c->findOne();
		$this->assertEquals(new MongoInt64('1234567890123'), $x['int64']);
	}

	function testInsertIntMongo64LongAsObjectC()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(123456789012345)));
		$x = $c->findOne();
		$this->assertEquals(new MongoInt64('1'), $x['int64']);
	}

	/** objects with string ctor */
	function testInsertStringMongo32()
	{
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32('1234567890')));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int32']);
	}

	function testInsertStringMongo32B()
	{
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32('123456789012345')));
		$x = $c->findOne();
		$this->assertSame(PHP_INT_MAX, $x['int32']);
	}

	function testInsertStringMongo32NativeLong()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32('1234567890')));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int32']);
	}

	function testInsertStringMongo32NativeLongB()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32('123456789012345')));
		$x = $c->findOne();
		$this->assertSame(PHP_INT_MAX, $x['int32']);
	}

	function testInsertStringMongo32LongAsObject()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32('1234567890')));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int32']);
	}

	function testInsertStringMongo32LongAsObjectB()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int32' => new MongoInt32('123456789012345')));
		$x = $c->findOne();
		$this->assertSame(PHP_INT_MAX, $x['int32']);
	}

	function testInsertStringMongo64()
	{
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64('1234567890')));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame(1234567890.0, $x['int64']);
	}

	function testInsertStringMongo64B()
	{
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64('123456789012345')));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame(123456789012345.0, $x['int64']);
	}

	/**
	 * @expectedException MongoCursorException
	 */
	function testInsertStringMongo64NativeLong()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64('1234567890')));
		$x = $c->findOne();
		$this->assertSame(1234567890, $x['int64']);
	}

	/**
	 * @expectedException MongoCursorException
	 */
	function testInsertStringMongo64NativeLongB()
	{
		ini_set('mongo.native_long', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64('123456789012345')));
		$x = $c->findOne();
		$this->assertSame(123456789012345, $x['int64']);
	}

	function testInsertStringMongo64LongAsObject()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64('1234567890')));
		$x = $c->findOne();
		$this->assertEquals(new MongoInt64('1234567890'), $x['int64']);
		$this->assertSame('1234567890', $x['int64']->__toString());
	}

	function testInsertStringMongo64LongAsObjectB()
	{
		ini_set('mongo.long_as_object', 1);
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64('123456789012345')));
		$x = $c->findOne();
		$this->assertEquals(new MongoInt64('123456789012345'), $x['int64']);
		$this->assertSame('123456789012345', $x['int64']->__toString());
	}

	/** Tests for object generation */
	function testObjectCreationInt32()
	{
		$a = new MongoInt32('1234567890');
		$this->assertSame('1234567890', $a->__toString());

		$a = new MongoInt32('1234567890123456');
		$this->assertSame('1234567890123456', $a->__toString());

		$a = new MongoInt32('123456789012345678901234567890');
		$this->assertSame('123456789012345678901234567890', $a->__toString());
	}

	function testObjectCreationInt64()
	{
		$a = new MongoInt64('1234567890');
		$this->assertSame('1234567890', $a->__toString());

		$a = new MongoInt64('1234567890123456');
		$this->assertSame('1234567890123456', $a->__toString());

		$a = new MongoInt64('123456789012345678901234567890');
		$this->assertSame('123456789012345678901234567890', $a->__toString());
	}

	/** Tests for things outside of the int64 range */
	function testInsertIntMongo64Big()
	{
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64(123456789012345678901234567890)));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame((double) 1, $x['int64']);
	}

	function testInsertStringMongo64Big()
	{
		$c = $this->object;
		$c->insert(array('int64' => new MongoInt64('123456789012345678901234567890')));
		$x = $c->findOne();
		$this->assertType('float', $x['int64']);
		$this->assertSame(9223372036854775807, $x['int64']);
	}
}

?>
