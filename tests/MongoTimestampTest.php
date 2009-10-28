<?php

require_once 'PHPUnit/Framework.php';

class MongoTimestampTest extends PHPUnit_Framework_TestCase
{

    protected $object;

    public function setUp() {
      $this->object = $this->sharedFixture->selectCollection("phpunit", "ts");
    }

    public function testBasic() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped("No implicit __toString in 5.1");
            return;
        }
        $ts = new MongoTimestamp();
        $now = time();

        $this->assertEquals("$ts", "$now");

        $c = $this->object;
        $c->drop();
        $c->insert(array("ts" => $ts));
        $x = $c->findOne();
        $this->assertNotNull($x);
        $this->assertTrue($x['ts'] instanceof MongoTimestamp);
        $this->assertEquals("".$x['ts'], "$ts");
    }

    public function testParam() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped("No implicit __toString in 5.1");
            return;
        }

        $c = $this->object;
        $c->drop();
        
        $n = new MongoTimestamp(123456789);
        $this->assertEquals("123456789", "$n");
      
        $c->insert(array("ts" => $n));
        $x = $c->findOne();
        $this->assertEquals("123456789", $x['ts']."");
        $this->assertEquals($n->inc, $x['ts']->inc);
    }

    public function testParam2() {
        if (preg_match($this->sharedFixture->version_51, phpversion())) {
            $this->markTestSkipped("No implicit __toString in 5.1");
            return;
        }

        $c = $this->object;
        $c->drop();
        
        $n = new MongoTimestamp(60, 30);
        $this->assertEquals("60", "$n");
        $this->assertEquals(60, $n->sec);
        $this->assertEquals(30, $n->inc);

        $n = new MongoTimestamp("60", "30");
        $this->assertEquals("60", "$n");
        $this->assertEquals(60, $n->sec);
        $this->assertEquals(30, $n->inc);

        $n = new MongoTimestamp("foo", "bar");
        $this->assertEquals(0, $n->sec);
        $this->assertEquals(0, $n->inc);

        $n = new MongoTimestamp(60.123, "40");
        $this->assertEquals(60, $n->sec);
        $this->assertEquals(40, $n->inc);
      
        $c->insert(array("ts" => $n));
        $x = $c->findOne();
        $this->assertEquals("60", $x['ts']."");
        $this->assertEquals($n->inc, $x['ts']->inc);
    }

    public function testFields() {
      $ts = new MongoTimestamp(0,0);
      $this->assertEquals(0, $ts->sec);
      $this->assertEquals(0, $ts->inc);

      $ts = new MongoTimestamp(12345, 678910);
      $this->assertEquals(12345, $ts->sec);
      $this->assertEquals(678910, $ts->inc);

      $ts1 = new MongoTimestamp;
      $ts2 = new MongoTimestamp;
      $this->assertEquals($ts1->inc+1, $ts2->inc);
    }
}

?>
