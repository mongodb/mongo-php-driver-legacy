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

    public function testIdInsert() {
        $c = $this->sharedFixture->selectCollection("phpunit", "c");

        $a = array('_id' => 1);
        $c->insert($a);
        $this->assertArrayHasKey('_id', $a);

        $c->drop();
        $a = array('x' => 1, '_id' => new MongoId());
        $id = (string)$a['_id'];
        $c->insert($a);
        $x = $c->findOne();

        $this->assertArrayHasKey('_id', $x);
        $this->assertEquals((string)$x['_id'], $id);
    }

    public function testFatalClone() {
        $output = "";
        $exit_code = 0;
        exec("php tests/fatal1.php", $output, $exit_code);
        $this->assertEquals("Fatal error: Trying to clone an uncloneable object of class MongoId in /home/k/gitroot/pecl/mongo/tests/fatal1.php on line 4", $output[1]); 
        $this->assertEquals($exit_code, 255);

        exec("php tests/fatal2.php", $output, $exit_code);
        $this->assertEquals("Fatal error: Trying to clone an uncloneable object of class MongoCursor in /home/k/gitroot/pecl/mongo/tests/fatal2.php on line 5", $output[3]); 
        $this->assertEquals($exit_code, 255);
    }
}
?>
