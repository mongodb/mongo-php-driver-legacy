<?php
require_once 'PHPUnit/Framework.php';

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
        $db = new MongoDB($this->sharedFixture, "phpunit");
        $this->object = $db->selectCollection('c');
        $this->object->drop();
    }
    
    public function testFsyncOpt() {
        $result = $this->object->insert(array("x" => 1), array("fsync" => 1));
        $this->assertArrayHasKey("fsyncFiles", $result);
    }

    /**
     * @expectedException MongoCursorException 
     */
    public function testSafeW() {
        $result = $this->object->insert(array("category" => "fruit", "name" => "apple"), array("safe" => 3));
        $this->assertArrayHasKey("wtime", $result);
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
}

?>
