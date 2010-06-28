<?php
require_once 'PHPUnit/Framework.php';

class RegressionTest1 extends PHPUnit_Framework_TestCase
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
    }

    /**
     * @expectedException MongoException
     */
    public function testTinyInsert2() {
        $c = $this->sharedFixture->selectCollection("phpunit", "c");
        $c->drop();

        $c->remove();
        $c->insert(array());
        $obj = $c->findOne();
        $this->assertEquals($obj, NULL);
    }

    public function testIdInsert() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped("No implicit __toString in 5.1");
            return;
        }

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
      if (phpversion() == "5.2.9") {
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
    }

    public function testRealloc() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped('Cannot open files w/out absolute path in 5.1.');
            return;
        }

        $db = $this->sharedFixture->selectDB('webgenius');
        $tbColl = $db->selectCollection('Text_Block');
        
        $text = file_get_contents('tests/mongo-bug.txt');
      
        $arr = array('text' => $text,);
        $tbColl->insert($arr);
    }

    public function testIdRealloc() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped('Cannot open files w/out absolute path in 5.1.');
            return;
        }

        $db = $this->sharedFixture->selectDB('webgenius');
        $tbColl = $db->selectCollection('Text_Block');

        $text = file_get_contents('tests/id-alloc.txt');
        $arr = array('text' => $text, 'id2' => new MongoId());
        $tbColl->insert($arr);
    }

    public function testSafeInsertRealloc() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped('Cannot open files w/out absolute path in 5.1.');
            return;
        }

        $db = $this->sharedFixture->selectDB('webgenius');
        $tbColl = $db->selectCollection('Text_Block');

        $text = file_get_contents('tests/id-alloc.txt');
        $arr = array('text' => $text);
        $x = $tbColl->insert($arr, true);
        $this->assertEquals($x['err'], null);
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

    public function testCursorCount() {
        $c = $this->sharedFixture->selectCollection('x', 'y');
        $c->drop();

        for($i=0; $i < 10; $i++) {
            $c->insert(array('foo'=>'bar'));
        }

        $cursor = $c->find();
        $this->assertEquals(10, $cursor->count());

        $cursor->limit(2);
        $this->assertEquals(10, $cursor->count());
        $this->assertEquals(2, $cursor->count(true));
    }

    public function testCursorConversion() {
        $c = $this->sharedFixture->selectCollection('x', 'y');
        $c->drop();

        for($i=0; $i < 10; $i++) {
            $c->insert(array('foo'=>'bar'));
        }

	$cursor = $c->find();
	foreach ($cursor as $k=>$v) {
	    $this->assertTrue(is_string($k));
	    $this->assertTrue($v['_id'] instanceof MongoId);
	}

	$c->remove();
	$c->insert(array("_id" => 20));

	$cursor = $c->find();
	$cursor->next();
	$this->assertTrue(is_string($cursor->key()));
	$v = $cursor->current();
	$this->assertTrue(is_int($v['_id']));
    }

    public function testCountReturnsInt() {
      $c = $this->sharedFixture->selectCollection('x', 'y');
      $c->drop();

      $this->assertTrue(is_int($c->count()));
      $cursor = $c->find();
      $this->assertTrue(is_int($cursor->count()));

      for($i=0; $i < 10; $i++) {
        $c->insert(array('foo'=>'bar'));
      }

      $cursor = $c->find();
      $x = $cursor->count();
      $this->assertTrue(is_int($x), $x);

      $cursor->limit(4);
      $this->assertTrue(is_int($cursor->count()));

      $this->assertTrue(is_int($c->count()));
    }

    public function testNestedArray() {
      $count = 1500;
      $mongo = new Mongo();
      $mongoDB = $mongo->selectDb('testdb');
      $mongoCollection = $mongoDB->selectCollection('testcollection');
      $values = array();
      for($i = 0; $i < $count; $i++) {
        $values[] = new MongoId($i);
      }
      $mongoCursor = $mongoCollection->find(array('_id' => array('$in' => $values)));
      while ($mongoCursor->hasNext()) {
        $mongoCursor->getNext();
      } 
    }

    public function testEnsureIndex() {
      $mongoConnection = new Mongo('127.0.0.1:27017');
      $collection = $mongoConnection->selectCollection("debug", "col1");
      $data = array("field"=>"some data","date"=>date("Y-m-s"));
      $this->assertEquals(true, $collection->save($data));
      
      $tmp = array("date" => 1);
      $this->assertEquals(1, $tmp['date']);
      $this->assertEquals(true, $collection->ensureIndex($tmp));
      $this->assertEquals(1, $tmp['date']);
    }

    public function testGridFSProps() {

      $m = new Mongo();
      $grid = $m->foo->getGridFS();

      $x = new MongoGridFSFile($grid, array());
      $this->assertNull($x->getFilename());
      
      $this->assertNull($x->getSize());
    }

    public function testCryllic() {
      $c = $this->sharedFixture->phpunit->c;
      try { 
        $c->insert(array("x" => "\xC3\x84"));
      }
      catch (MongoException $e) {
        $this->assertTrue(false, $e);
      }
    }

    public function testEmptyQuery() {
      $c = $this->sharedFixture->phpunit->c;
      $c->drop();

      for ($i=0; $i<10; $i++) {
        $c->insert(array("x" => $i));
      }
      
      $cursor = $c->find(array(), array("x" => 1))->sort(array("x" => 1));

      $count = 0;
      foreach ($cursor as $doc) {
        $count++;
      }
      $this->assertEquals(10, $count);
    }

    /**
     * @expectedException MongoException
     */
    public function testNonUTF81() {
      $c = $this->sharedFixture->phpunit->c;
      $c->insert(array("x" => "\xFE"));
    }

    public function testNonUTF82() {
      $c = $this->sharedFixture->phpunit->c;
      ini_set("mongo.utf8", 0);
      $c->insert(array("x" => "\xFE"));
      ini_set("mongo.utf8", 1);
      $c->drop();
    }

    /**
     * @expectedException MongoConnectionException
     */
    public function testNoPassword1() {
      new Mongo("mongodb://admin@anyhost-and-nonexistent");
    }

    /**
     * @expectedException MongoConnectionException
     */
    public function testNoPassword2() {
      new Mongo("mongodb://admin@anyhost-and-nonexistent:foo");
    }

    /**
     * @expectedException MongoConnectionException
     */
    public function testNoPassword3() {
      new Mongo("mongodb://admin@anyhost-and-nonexistent:27017");
    }

    /**
     * @expectedException MongoConnectionException
     */
    public function testNoPassword4() {
      new Mongo("mongodb://@anyhost-and-nonexistent:27017");
    }

    /**
     * @expectedException MongoConnectionException
     */
    public function testNoPassword5() {
      new Mongo("mongodb://:@anyhost-and-nonexistent");
    }

    public function testFatalRecursion() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped('annoying output in 5.1.');
            return;
        }

        $output = "";
        $exit_code = 0;
        exec("php tests/fatal4.php", $output, $exit_code);
        $msg = "Fatal error: Nesting level too deep";

        if (count($output) > 0) {
            $this->assertEquals($msg, substr($output[1], 0, strlen($msg)), json_encode($output)); 
        }
    }

    public function testStaticDtor() {
        $f = new Foo2();
        $f->f();
    }

    public function testGetMore() {
      $c = $this->sharedFixture->phpunit->c;
      
      for($i=0; $i<500; $i++) {
        $c->insert(array("x" => new MongoDate(), "count" => $i, "my string" => "doo dee doo"));
      }

      $cursor = $c->find();
      foreach ($cursor as $v) {
      }
    }

    /**
     * @expectedException MongoException
     */
    public function testNonUTF8Embed() {
      $this->sharedFixture->phpunit->c->insert(array('x'=>array("y" => "\xFF"), "y" => array()));
    }

    /**
     * @expectedException MongoConnectionException
     */
    public function testInvalidConnectionSyntax() {
      $m = new Mongo("mongodb://name:password@localhost/");
    }

    /**
     * @expectedException MongoException
     */
    public function testTooBigInsert() {
      if (preg_match($this->sharedFixture->version_51, phpversion())) {
        $this->markTestSkipped('annoying output in 5.1.');
        return;
      }

      $contents = file_get_contents('tests/pycon-poster.pdf');

      $arr = array();
      for ($i=0; $i<7; $i++) {
        $arr[] = array("content" => new MongoBinData($contents), "i" => $i);
      }

      $this->sharedFixture->phpunit->c->batchInsert($arr);
    }
}

class Foo2 {
  static $c;
  public function f() {
    $m = new Mongo();
    $db = new MongoDB($m, 'foo');
    Foo2::$c = new MongoCollection($db, 'bar');
  }
}

?>
