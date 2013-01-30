<?php
class MongoCollectionTest2 extends PHPUnit_Framework_TestCase
{
    /**
     * @var    MongoCollection
     * @access protected
     */
    protected $object;

    /**
     * @access protected
     */
    protected function setUp()
    {
        $m = new Mongo();
        $db = new MongoDB($m, "phpunit");
        $this->object = $db->selectCollection('c');
        $this->object->drop();
    }

    public function testFsyncOpt() {
        $result = $this->object->insert(array("x" => 1), array("fsync" => 1));
        $this->assertArrayHasKey("err", $result, json_encode($result));
    }

    public function testSafeW() {
        $result = $this->object->insert(array("category" => "fruit", "name" => "apple"), array("safe" => 1));
        $this->assertArrayHasKey("ok", $result, json_encode($result));
    }

    public function testFields() {
        $this->object->insert(array("x" => array(1,2,3,4,5)));
        $results = $this->object->find(array(),array("x" => array('$slice' => 3)))->getNext();
        $r = $results['x'];

        $this->assertTrue(array_key_exists(0, $r));
        $this->assertTrue(array_key_exists(1, $r));
        $this->assertTrue(array_key_exists(2, $r));
        $this->assertFalse(array_key_exists(3, $r));
        $this->assertFalse(array_key_exists(4, $r));
    }

    /**
     * @expectedException MongoException
     */
    public function testIndexNameLen1() {
      $this->object->ensureIndex(array("x" => 1), array("name" => "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"));
    }

    /**
     * @expectedException MongoException
     */
    public function testIndexNameLen2() {
      $this->object->ensureIndex(array("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" => 1));
    }

    public function testTimeout() {
        $this->object->insert(array("x" => 1), array("safe" => true, "timeout" => -1));
        $this->object->insert(array("x" => 1), array("safe" => true, "timeout" => 30));
        $this->object->insert(array("x" => 1), array("safe" => true, "timeout" => 1000));
    }

    /**
     * @expectedException Exception
     */
    public function testCtor() {
        $db = $this->object->db;
        $c = new MongoCollection($db, "");
    }
}

?>
