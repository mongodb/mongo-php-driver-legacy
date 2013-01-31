--TEST--
MongoDB Standalone
--SKIPIF--
<?php
// We execute the generic/ tests first under STANDALONE mode, no need to execute them again
// If we however have a TESTS environment variable we can't accurately detect if they have been executed already
// This allows "make test TESTS=tests/standalone" to still execute generic/ via redirect
// While "make test" will not redirect it, but execute them when running the generic/ folder
if (!isset($_ENV["TESTS"])) {
    exit("skip Already executed the generic/ tests");
}
// Force standalone mode
$_ENV["MONGO_SERVER"] = "STANDALONE";
require_once "tests/utils/standalone.inc";
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "STANDALONE"),
    'TESTS' => "tests/generic",
);

