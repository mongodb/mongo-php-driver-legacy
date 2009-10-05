<?php

require_once 'PHPUnit/Framework.php';

class MongoTimestampTest extends PHPUnit_Framework_TestCase
{
    public function testBasic() {
      $ts = new MongoTimestamp();
      $now = time();

      $this->assertEquals("$ts", "$now");

      $c = $this->sharedFixture->selectCollection("phpunit", "ts");
      $c->drop();
      $c->insert(array("ts" => $ts));
      $x = $c->findOne();
      $this->assertNotNull($x);
      $this->assertTrue($x['ts'] instanceof MongoTimestamp);
      $this->assertEquals("".$x['ts'], "$ts");
    }

    public function testParam() {
      $c = $this->sharedFixture->selectCollection("phpunit", "ts");
      $c->drop();

      $n = new MongoTimestamp(123456789);
      $this->assertEquals("123456789", "$n");
      
      $c->insert(array("ts" => $n));
      $x = $c->findOne();
      $this->assertEquals("123456789", $x['ts']."");
    }
}

?>
