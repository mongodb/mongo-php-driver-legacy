<?php
require_once 'PHPUnit/Framework.php';

/**
 * Test class for MongoDate
 */
class MongoDateTest extends PHPUnit_Framework_TestCase
{
    public function testBasic() {
        $t = time();

        $d1 = new MongoDate();
        $d2 = new MongoDate($d1->sec, $d1->usec);
        $this->assertEquals($d1->sec, $d2->sec);
        $this->assertEquals($d1->usec, $d2->usec);
        $this->assertEquals($t, $d1->sec);

        $d3 = new MongoDate('2009-05-01 00:00:00');
        $this->assertEquals(1241150400, $d3->sec);
        $this->assertEquals(0, $d3->usec);

        $t = strtotime('2009-05-01 00:00:00');
        $d4 = new MongoDate($t); 
        $this->assertEquals(1241150400, $d3->sec);
        $this->assertEquals(0, $d3->usec);
    }

    public function testFormat() {
        $d = new MongoDate('2009-05-01 00:00:00');
        $this->assertEquals("Friday", $d->format("l"));
    }
}

?>
