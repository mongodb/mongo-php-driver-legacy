--TEST--
Test for PHP-1156: Negative values for timeouts should block indefinitely (non-streams)
--DESCRIPTION--
This test requires compiling with stream support disabled. Additionally, the
TEST_TIMEOUT environment variable must be set to >=63 when executing
run-tests.php, since the query in this test blocks for 61 seconds.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php if (MONGO_STREAMS) { echo "skip This test requires non-stream connections"; } ?>
skip Manual test
--FILE--
<?php
require_once "tests/utils/server.inc";

/* Since mcon does not depend on PHP, it does not log its applied timeouts and
 * we also cannot simply alter the default_socket_timeout INI option to shorten
 * the default wait time. Instead, we'll test that socket timeouts actually uses
 * the MONGO_CONNECTION_DEFAULT_CONNECT_TIMEOUT constant (60 seconds).
 */
$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('connectTimeoutMS' => -1, 'socketTimeoutMS' => -1));
echo "Connected\n";

echo "\nDropping collection\n";
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

echo "\nInserting two documents\n";
$collection->insert(array('x' => 1));
$collection->insert(array('x' => 2));

/* The server will timeout JavaScript execution after 60 seconds, so we need to
 * break this up into two separate executions.
 */
echo "\nExecuting find() with two 31.5-second sleeps and no timeout\n";
$cursor = $collection->find(array('$where' => 'sleep(31500) || true'));
iterator_to_array($cursor);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Connected

Dropping collection

Inserting two documents

Executing find() with two 31.5-second sleeps and no timeout
===DONE===
