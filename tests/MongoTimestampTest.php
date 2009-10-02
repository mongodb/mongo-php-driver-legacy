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
}

?>
