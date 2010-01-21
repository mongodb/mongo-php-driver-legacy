<?php
require_once 'PHPUnit/Framework.php';

class MongoDBRefTest extends PHPUnit_Framework_TestCase
{
    protected $object;

    protected function setUp()
    {
        $db = new MongoDB($this->sharedFixture, "phpunit");
        $this->object = $db->selectCollection('c');
        $this->object->drop();
    }

    /**
     * @expectedException MongoException 
     */
    public function testGetThrow1() {
	MongoDBRef::get($this->object->db, array('$ref' => 3, '$id' => 123));
    }

    /**
     * @expectedException MongoException 
     */
    public function testGetThrow2() {
        MongoDBRef::get($this->object->db, array('$ref' => "d", '$id' => 123, '$db' => 3));
    }


    public function testInvalidGet() {
      // some error cases... they should just return null
      $this->assertNull(MongoDBRef::get($this->object->db, null));
      $this->assertNull(MongoDBRef::get($this->object->db, array()));
      $this->assertNull(MongoDBRef::get($this->object->db, array('$ref' => "d")));
      $this->assertNull(MongoDBRef::get($this->object->db, array('$id' => "d")));
    }

    public function testGet() {
      $this->object->db->d->insert(array("_id" => 123, "greeting" => "hi"));
      $c = $this->sharedFixture->phpunit_temp->d;
      $c->drop();
      $c->insert(array("_id" => 123, "greeting" => "bye"), true);

      $x = MongoDBRef::get($this->object->db, array('$ref' => "d", '$id' => 123));
      $this->assertNotNull($x);
      $this->assertEquals("hi", $x['greeting'], json_encode($x));

      $x = MongoDBRef::get($this->object->db, array('$ref' => "d", '$id' => 123, '$db' => 'phpunit_temp'));
      $this->assertNotNull($x);
      $this->assertEquals("bye", $x['greeting'], json_encode($x));
    }


    public function testCreate() {
      $collection = "d";
      $id = 123;
      $db = "phpunit_temp";

      $ref = MongoDBRef::create($collection, $id);
      $this->assertEquals("d", $ref['$ref'], json_encode($ref));
      $this->assertEquals(123, $ref['$id'], json_encode($ref));
      $this->assertArrayNotHasKey('$db', $ref, json_encode($ref));

      $ref = MongoDBRef::create($collection, $id, $db);
      $this->assertEquals("d", $ref['$ref'], json_encode($ref));
      $this->assertEquals(123, $ref['$id'], json_encode($ref));
      $this->assertEquals("phpunit_temp", $ref['$db'], json_encode($ref));

      // test converting to strings
      $ref = MongoDBRef::create(1, 2, 3);
      $this->assertEquals("1", $ref['$ref'], json_encode($ref));
      $this->assertEquals(2, $ref['$id'], json_encode($ref));
      $this->assertEquals("3", $ref['$db'], json_encode($ref));

      // more for tracking this behavior than condoning it...
      $one = 1;
      $two = 2;
      $three = 3;
      $ref = MongoDBRef::create(1, 2, 3);
      $this->assertEquals("1", $one);
      $this->assertEquals(2, $two);
      $this->assertEquals("3", $three);
    }
}

?>
