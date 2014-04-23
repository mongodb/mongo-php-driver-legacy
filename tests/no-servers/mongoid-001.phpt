--TEST--
MongoID::isValid()
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$valid = array (
    '6D744bd1b8a5e5423d2af2a2',
    'FDAAB5413c671D9c6DCF3127',
    'e4B866b6450Fa51eB7FC599b',
    '65ba7dBFBd82DD91390ef16a',
    '967efA400fa77672Df5AdeCA',
    '2C8BEe58Ed0d75d7E2f58086',
    new MongoID(),
);

foreach($valid as $test) {
    if (MongoID::isValid($test)) {
        echo "VALID: ($test)\n";
    } else {
        echo "INVALID: ", var_export($test, true), "\n";
    }
}

$invalid = array (
    '',
    'f',
    'fe',
    'fe1',
    'fe16',
    'fe166',
    'fe166D',
    'fe166D1',
    'fe166D19',
    'fe166D19A',
    'fe166D19Aa',
    'fe166D19Aa5',
    'fe166D19Aa5e',
    'fe166D19Aa5e2',
    'fe166D19Aa5e2a',
    'fe166D19Aa5e2a0',
    'fe166D19Aa5e2a01',
    'fe166D19Aa5e2a012',
    'fe166D19Aa5e2a0121',
    'fe166D19Aa5e2a0121f',
    'fe166D19Aa5e2a0121f9',
    'fe166D19Aa5e2a0121f96',
    'fe166D19Aa5e2a0121f967',
    'fe166D19Aa5e2a0121f967B',
    'fe166D19Aa5e2a 0121f967B',
    'FaAf92CEAFf8CCC3353c8D0C7',
    'FaAf92CEAFf8CCC3353c8D0Cz',
    true,
    false,
    new stdclass,
    'asldkfj',
    1235,
    'asdf9875asdf6789gsdfasdf',
    'asdf9875asdf6789-sdfasdf',
);

foreach($invalid as $test) {
    if (MongoID::isValid($test)) {
        echo "VALID: (", var_export($test, true), ")\n";
    } else {
        echo "INVALID: (", var_export($test, true), ")\n";
    }
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
VALID: (6D744bd1b8a5e5423d2af2a2)
VALID: (FDAAB5413c671D9c6DCF3127)
VALID: (e4B866b6450Fa51eB7FC599b)
VALID: (65ba7dBFBd82DD91390ef16a)
VALID: (967efA400fa77672Df5AdeCA)
VALID: (2C8BEe58Ed0d75d7E2f58086)
VALID: (%s)
INVALID: ('')
INVALID: ('f')
INVALID: ('fe')
INVALID: ('fe1')
INVALID: ('fe16')
INVALID: ('fe166')
INVALID: ('fe166D')
INVALID: ('fe166D1')
INVALID: ('fe166D19')
INVALID: ('fe166D19A')
INVALID: ('fe166D19Aa')
INVALID: ('fe166D19Aa5')
INVALID: ('fe166D19Aa5e')
INVALID: ('fe166D19Aa5e2')
INVALID: ('fe166D19Aa5e2a')
INVALID: ('fe166D19Aa5e2a0')
INVALID: ('fe166D19Aa5e2a01')
INVALID: ('fe166D19Aa5e2a012')
INVALID: ('fe166D19Aa5e2a0121')
INVALID: ('fe166D19Aa5e2a0121f')
INVALID: ('fe166D19Aa5e2a0121f9')
INVALID: ('fe166D19Aa5e2a0121f96')
INVALID: ('fe166D19Aa5e2a0121f967')
INVALID: ('fe166D19Aa5e2a0121f967B')
INVALID: ('fe166D19Aa5e2a 0121f967B')
INVALID: ('FaAf92CEAFf8CCC3353c8D0C7')
INVALID: ('FaAf92CEAFf8CCC3353c8D0Cz')
INVALID: (true)
INVALID: (false)
INVALID: (stdClass::__set_state(array(
)))
INVALID: ('asldkfj')
INVALID: (1235)
INVALID: ('asdf9875asdf6789gsdfasdf')
INVALID: ('asdf9875asdf6789-sdfasdf')
===DONE===
