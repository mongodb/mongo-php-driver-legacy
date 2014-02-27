<?php
require_once "tests/utils/server.inc";

require_once "tests/utils/replicaset.inc";

if (getenv('SKIP_SLOW_TESTS')) {
    die('skip slow tests excluded by request');
}
