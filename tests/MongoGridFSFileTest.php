<?php
/**
 * Test class for Mongo.
 * Generated by PHPUnit on 2009-04-09 at 18:09:02.
 */
class MongoGridFSFileTest extends PHPUnit_Framework_TestCase
{
    /**
     * @var    MongoGridFSFile
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
        if (file_exists('tests/anotherfile')) {
            unlink('tests/anotherfile');
        }
        $m = new Mongo();
        $db = $m->selectDB('phpunit');
        $grid = $db->getGridFS();
        $grid->drop();
        $grid->storeFile('tests/somefile');
        $this->object = $grid->findOne();
        $this->object->start = memory_get_usage(true);
    }

    protected function tearDown() {
        $this->assertEquals($this->object->start, memory_get_usage(true));
    }

    public function testGetFilename() {
        $this->assertEquals($this->object->getFilename(), 'tests/somefile');
    }

    public function testGetSize() {
        $size = filesize('tests/somefile');
        $this->assertEquals($this->object->getSize(), $size);
    }

    public function testWrite() {
        $bytes = $this->object->write('tests/anotherfile');
        $this->assertEquals($bytes, 129);
        $this->assertEquals(filesize('tests/anotherfile'), 
                            $this->object->getSize());
    }
}
?>
