<?php

require_once 'PHPUnit/Framework.php';

class MongoTimestampTest extends PHPUnit_Framework_TestCase
{

    protected $object;

    public function setUp() {
      $this->object = $this->sharedFixture->selectCollection("phpunit", "ts");
    }

    public function testBasic() {
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
      $c = $this->object;
      $c->drop();

      $n = new MongoTimestamp(123456789);
      $this->assertEquals("123456789", "$n");
      
      $c->insert(array("ts" => $n));
      $x = $c->findOne();
      $this->assertEquals("123456789", $x['ts']."");
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
