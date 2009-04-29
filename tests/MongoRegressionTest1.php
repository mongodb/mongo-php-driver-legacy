<?php
require_once 'PHPUnit/Framework.php';

require_once 'Mongo.php';

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
     * Bug PHP-7
     */
    public function testConnect() {
        $x = mongo_connect("localhost:9923", "", "", false, false, false);
        $this->assertEquals($x, false);
    }

    /**
     * Bug PHP-9
     */
    public function testMem() {
        $c = $this->sharedFixture->selectCollection("phpunit", "c");
        $arr = array("test" => "1, 2, 3"); 
        $start = memory_get_usage(true);

        for($i = 1; $i < 20000; $i++) {
            $c->insert($arr);
        }
        $this->assertEquals($start, memory_get_usage(true));
        $c->drop();
    }
}
?>
