<?php
require_once 'PHPUnit/Framework.php';

class MongoRegressionTest1 extends PHPUnit_Framework_TestCase
{

    /**
     * Bug PHP-7
     * @expectedException MongoConnectionException
     */
    public function testConnectException1() {
        $x = new Mongo("localhost:9923");
    }

    /**
     * Bug PHP-9
     */
    public function testMem() {
        $c = $this->sharedFixture->selectCollection("phpunit", "c");
        $arr = array("test" => "1, 2, 3"); 
        $start = memory_get_usage(true);

        for($i = 1; $i < 2000; $i++) {
            $c->insert($arr);
        }
        $this->assertEquals($start, memory_get_usage(true));
        $c->drop();
    }

    public function testTinyInsert() {
        $c = $this->sharedFixture->selectCollection("phpunit", "c");
        $c->drop();

        $c->insert(array('_id' => 1));
        $obj = $c->findOne();
        $this->assertEquals($obj['_id'], 1);

        $c->remove();
        $c->insert(array());
        $obj = $c->findOne();
        $this->assertEquals($obj, NULL);
    }
}
?>
