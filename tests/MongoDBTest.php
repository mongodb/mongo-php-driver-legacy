<?php
/**
 * Test class for Mongo.
 * Generated by PHPUnit on 2009-04-09 at 18:09:02.
 */
class MongoDBTest extends PHPUnit_Framework_TestCase
{
    /**
     * @var    MongoDB
     * @access protected
     */
    protected $object;

    /**
     * Sets up the fixture, for example, opens a network connection.
     * This method is called before a test is executed.
     *
     * @access protected
     */
    protected function setUp()
    {
        $m = new Mongo();
        $this->object = new MongoDB($m, "phpunit");
        $this->object->start = memory_get_usage(true);
    }

    protected function tearDown() {
      //        $this->assertEquals($this->object->start, memory_get_usage(true));
    }

    public function testGetGridFS() {
        if (preg_match("/5\.1\../", phpversion())) {
            $this->markTestSkipped("No implicit __toString in 5.1");
            return;
        }

        $grid = $this->object->getGridFS();

        $this->assertTrue($grid instanceof MongoGridFS);
        $this->assertTrue($grid instanceof MongoCollection);

        $this->assertEquals((string)$grid, "phpunit.fs.files");
        $this->assertEquals((string)$grid->chunks, "phpunit.fs.chunks");

        $grid = $this->object->getGridFS("foo");
        $this->assertEquals((string)$grid, "phpunit.foo.files");
        $this->assertEquals((string)$grid->chunks, "phpunit.foo.chunks");

        $grid = $this->object->getGridFS("foo", "bar");
        $this->assertEquals((string)$grid, "phpunit.foo.files");
        $this->assertEquals((string)$grid->chunks, "phpunit.foo.chunks");
    }


    public function testDrop() {
      $r = $this->object->drop();
      $this->assertEquals(true, (bool)$r['ok'], json_encode($r));
    }

    public function testRepair() {
      $r = $this->object->repair();
      $this->assertEquals(true, (bool)$r['ok'], json_encode($r));
      $r = $this->object->repair(true);
      $this->assertEquals(true, (bool)$r['ok'], json_encode($r));
      $r = $this->object->repair(true, true);
      $this->assertEquals(true, (bool)$r['ok'], json_encode($r));
    }

    public function testSelectCollection() {
        if (preg_match("/5\.1\../", phpversion())) {
            $this->markTestSkipped("No implicit __toString in 5.1");
            return;
        }

        $this->assertEquals((string)$this->object->selectCollection('x'), 'phpunit.x');
        $this->assertEquals((string)$this->object->selectCollection('..'), 'phpunit...');
        $this->assertEquals((string)$this->object->selectCollection('a b c'), 'phpunit.a b c');
    }

    public function testListCollections() {
        $ns = $this->object->selectCollection('system.namespaces');

        for($i=0;$i<10;$i++) {
            $c = $this->object->selectCollection("x$i");
            $c->insert(array("foo" => "bar"));
        }

        $list = $this->object->listCollections();
        for($i=0;$i<10;$i++) {
            $this->assertTrue($list[$i] instanceof MongoCollection);
            if (!preg_match("/5\.1\../", phpversion())) {
              $this->assertTrue(in_array("phpunit.x$i", $list));
            }
        }
    }

    public function testCreateDBRef() {
        $ref = $this->object->createDBRef('foo.bar', array('foo' => 'bar'));
        $this->assertEquals($ref, NULL);

        $arr = array('_id' => new MongoId());
        $ref = $this->object->createDBRef('foo.bar', $arr);
        $this->assertNotNull($ref);
        $this->assertTrue(is_array($ref));

        $arr = array('_id' => 1);
        $ref = $this->object->createDBRef('foo.bar', $arr);
        $this->assertNotNull($ref);
        $this->assertTrue(is_array($ref));

        $ref = $this->object->createDBRef('foo.bar', new MongoId());
        $this->assertNotNull($ref);
        $this->assertTrue(is_array($ref));

        $id = new MongoId();
        $ref = $this->object->createDBRef('foo.bar', array('_id' => $id, 'y' => 3));
        $this->assertNotNull($ref);
        $this->assertEquals((string)$id, (string)$ref['$id']);
    }

    public function testGetDBRef() {
        $c = $this->object->selectCollection('foo');
        $c->drop();
        for($i=0;$i<50;$i++) {
            $c->insert(array('x' => rand()));
        }
        $obj = $c->findOne();

        $ref = $this->object->createDBRef('foo', $obj);
        $obj2 = $this->object->getDBRef($ref);

        $this->assertNotNull($obj2);
        $this->assertEquals($obj['x'], $obj2['x']);
    }

    public function testExecute() {
        $ret = $this->object->execute('4+3*6');
        $this->assertEquals($ret['retval'], 22, json_encode($ret));

        $ret = $this->object->execute(new MongoCode('function() { return x+y; }', array('x' => 'hi', 'y' => 'bye')));
        $this->assertEquals($ret['retval'], 'hibye', json_encode($ret));

        $ret = $this->object->execute(new MongoCode('function(x) { return x+y; }', array('y' => 'bye')), array('bye'));
        $this->assertEquals($ret['retval'], 'byebye', json_encode($ret));
    }

    public function testCreateRef() {
        $ref = MongoDBRef::create("x", "y");
        $this->assertEquals('x', $ref['$ref']);
        $this->assertEquals('y', $ref['$id']);
    }

    public function testIsRef() {
        $this->assertFalse(MongoDBRef::isRef(array()));
        $this->assertFalse(MongoDBRef::isRef(array('$ns' => 'foo', '$id' => 'bar')));
        $ref = $this->object->createDBRef('foo.bar', array('foo' => 'bar'));
        $this->assertEquals(NULL, $ref);

        $ref = array('$ref' => 'blog.posts', '$id' => new MongoId('cb37544b9dc71e4ac3116c00'));
        $this->assertTrue(MongoDBRef::isRef($ref));
    }

    public function testLastError() {
        $this->object->resetError();
        $err = $this->object->lastError();
        $this->assertEquals(null, $err['err'], json_encode($err));
        $this->assertEquals(0, $err['n'], json_encode($err));
        $this->assertEquals(true, (bool)$err['ok'], json_encode($err));

        $this->object->forceError();
        $err = $this->object->lastError();
        $this->assertNotNull($err['err'], json_encode($err));
        $this->assertEquals($err['n'], 0, json_encode($err));
        $this->assertEquals((bool)$err['ok'], true, json_encode($err));
    }

    public function testResetError() {
        $this->object->resetError();
        $err = $this->object->lastError();
        $this->assertEquals($err['err'], null);
        $this->assertEquals($err['n'], 0);
        $this->assertEquals((bool)$err['ok'], true);
    }

    public function testForceError() {
        $this->object->forceError();
        $err = $this->object->lastError();
        $this->assertNotNull($err['err']);
        $this->assertEquals($err['n'], 0);
        $this->assertEquals((bool)$err['ok'], true);
    }

    public function testW() {
      $this->assertEquals(1, $this->object->w);
      $this->assertEquals(10000, $this->object->wtimeout);

      $this->object->w = 4;
      $this->object->wtimeout = 60;

      $this->assertEquals(4, $this->object->w);
      $this->assertEquals(60, $this->object->wtimeout);
   }
}
?>
