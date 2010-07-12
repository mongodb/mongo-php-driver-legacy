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
      $temp = $this->object->wtimeout;
      $this->object->wtimeout = 1000;

      $this->object->insert(array("category" => "fruit", "name" => "apple"), array("safe" => 3));

      $this->object->wtimeout = $temp;
    }
}

?>
