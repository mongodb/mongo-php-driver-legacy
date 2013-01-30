<?php
    
function query() {
    global $m;
    
    //echo "querying master: $m\n";
    
    $c = $m->foo->bar;
    $cursor = $c->find();
    $counter = 0;
    try {
        foreach ($cursor as $v) {
            $counter++;
        }
        $info = $cursor->info();
        //echo "iterated through $counter results from ".$info['server']."\n";
    }
    catch (MongoCursorException $e) {
        //echo "EXCEPTION (query): ".$e->getMessage()."\n";
    }
}

function querySlave() {
    global $m;
    //echo "querying slave ".$m->getSlave().", $m\n";

    $c = $m->foo->bar;
    $cursor = $c->find()->slaveOkay();
    $counter = 0;
    try {
        foreach ($cursor as $v) {
            $counter++;
        }
        $info = $cursor->info();
        //echo "iterated through $counter results from ".$info['server']."\n";
    }
    catch (MongoCursorException $e) {
        //echo "EXCEPTION (query): ".$e->getMessage()."\n";
    }
}

function insert() {
    global $m;
    
    $c = $m->foo->bar;
    try {
        $c->insert(array("x"=>1, "y"=>new MongoDate(), "z"=>"n"), array("safe"=>true));
    }
    catch(MongoException $e) {
        //echo "EXCEPTION (insert): ".$e->getMessage()."\n";
    }
}

function remove() {
    global $m;

    try {
        $m->foo->bar->remove(array(), array("safe"=>true));
    }
    catch (MongoException $e) {
        echo $e->getMessage()."\n";
    }
}

function stepDown() {
    global $m;
    
    echo "stepping down master: $m\n";
    try {
        $result = $m->admin->command(array("replSetStepDown" => 1));
        var_dump($result);
    }
    catch (MongoCursorException $e) {
        echo "EXCEPTION: ".$e->getMessage()."\n";
    }
}

function blind($s) {
    echo "blinding $s\n";
    try {
        $result = $s->admin->command(array("replSetTest" => 1, "blind" => true));
        var_dump($result);
    }
    catch(MongoCursorException $e) {
        echo "EXCEPTION: ".$e->getMessage()."\n";
    }
}

function unblind($s) {
    echo "unblinding $s\n";
    try {
        $result = $s->admin->command(array("replSetTest" => 1, "blind" => false));
        var_dump($result);
    }
    catch(MongoCursorException $e) {
        echo "EXCEPTION: ".$e->getMessage()."\n";
    }
}

$m = new Mongo("mongodb://localhost:27017", array("replicaSet" => true));

$server = array(new Mongo("localhost:27017"),
                new Mongo("localhost:27018"),
                new Mongo("localhost:27019"));

$count = 0;
while (true) {
    usleep(100);
    
    $op = rand(0, 1000);
    
    switch ($op) {
    case 0:
    case 1:
    case 2:
        blind($server[$op]);
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        unblind($server[$op%3]);
        break;
    case 9:
        stepDown();
        break;
    case 10:
        remove();
        break;
    default:
        if ($op % 3 == 0) {
            query();
        }
        else if ($op % 3 == 1) {
            querySlave();
        }
        else {
            insert();
        }
    }

    $count++;
    if ($count % 1000 == 0) {
        echo "Memory: ".memory_get_usage(true)."\n";
    }
}
    
?>