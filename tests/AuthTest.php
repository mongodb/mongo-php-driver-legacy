<?php
require_once 'PHPUnit/Framework.php';

class AuthTest extends PHPUnit_Framework_TestCase
{
    /**
     * @var    MongoDB
     * @access protected
     */
    protected $object;

    protected function setUp()
    {
        $m = new Mongo();
	$this->object = $m->selectDB("admin");
    }

    public function testAuthBasic() {
	$ok = $this->object->authenticate("testUser", "testPass");
	$this->assertEquals($ok['ok'], 1, json_encode($ok));

	$ok = $this->object->authenticate("testUser", "bar");
	$this->assertEquals($ok['ok'], 0, json_encode($ok));
    }

}

?>
