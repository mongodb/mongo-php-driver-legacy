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
        $unclonable = "Fatal error: Trying to clone an uncloneable object";

        if (count($output) > 0) {
            $this->assertEquals($unclonable, substr($output[1], 0, strlen($unclonable)), json_encode($output)); 
        }
        $this->assertEquals(255, $exit_code);

        exec("php tests/fatal2.php", $output, $exit_code);
        if (count($output) > 0) {
            $this->assertEquals($unclonable, substr($output[3], 0, strlen($unclonable)), json_encode($output)); 
        }
        $this->assertEquals(255, $exit_code);
    }

    public function testRealloc() {
        $db = $this->sharedFixture->selectDB('webgenius');
        $tbColl = $db->selectCollection('Text_Block');
        
        $text = file_get_contents('tests/mongo-bug.txt');
      
        $arr = array('text' => $text,);
        $tbColl->insert($arr);
    }

    public function testIdRealloc() {
        $db = $this->sharedFixture->selectDB('webgenius');
        $tbColl = $db->selectCollection('Text_Block');

        $text = file_get_contents('tests/id-alloc.txt');
        $arr = array('text' => $text, 'id2' => new MongoId());
        $tbColl->insert($arr);
    }

    public function testMongoEmptyObj() {
        $c = $this->sharedFixture->selectCollection('x', 'y');
        $c->drop();

        $c->insert(array('x' => array(), 'y' => new MongoEmptyObj()));
        $c->update(array(), array('$push' => array('x' => 'foo')));
        $c->update(array(), array('$push' => array('y' => 'bar')));

        $x = $c->findOne();
        $this->assertTrue(empty($x['y']));
        $this->assertEquals(1, count($x['x'])); 
        $this->assertEquals('foo', $x['x'][0]);
    }

    public function testForEachKey() {
        $c = $this->sharedFixture->selectCollection('x', 'y');
        $c->drop();

        $c->insert(array('_id' => "xsf0", 'x' => 1));
        $c->insert(array('_id' => 1, 'x' => 2));
        $c->insert(array('_id' => true, 'x' => 3));
        $c->insert(array('_id' => null, 'x' => 4));
        $c->insert(array('_id' => new MongoId(), 'x' => 5));
        $c->insert(array('_id' => new MongoDate(), 'x' => 6));

        $cursor = $c->find()->sort(array('x'=>1));
        $data = array();
        foreach($cursor as $k=>$v) {
            $data[] = $k;
        }

        $this->assertEquals('xsf0', $data[0]);
        $this->assertEquals('1', $data[1]);
        $this->assertEquals('1', $data[2]);
        $this->assertEquals('', $data[3]);
        $this->assertEquals(24, strlen($data[4]), "key: ".$data[4]);
        $this->assertEquals(21, strlen($data[5]), "key: ".$data[5]);
    }

    public function testIn() {
        $c = $this->sharedFixture->selectCollection('x', 'y');
        $x = $c->findOne(array('oldId' =>array('$in' =>array ())));
        if ($x != NULL) {
            $this->assertArrayNotHasKey('$err', $x, json_encode($x));
        }
    }

    public function testBatchInsert() {
        $c = $this->sharedFixture->selectCollection('x', 'y');
        $c->drop();

        $a = array();
        for($i=0; $i < 10; $i++) {
            $a[] = array('time' => new MongoDate(), 'x' => $i, "label" => "ooo$i");
        }

        $c->batchInsert($a);

        for ($i=0; $i <10; $i++) {
            $this->assertArrayHasKey('_id', $a[$i], json_encode($a));
        }
    }

    /**
     * Mongo::toString() was destroying Mongo::server
     */
    public function testMongoToString() {
        $m = new Mongo();
        $str1 = $m->__toString();
        $str2 = $m->__toString();
        $this->assertEquals("localhost:27017", $str2);
        $this->assertEquals($str1, $str2);
    }

}
?>
