--TEST--
MongoCollection->insert() #001
--SKIPIF--
<?php $needs = "2.5.4"; ?>
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/stream-notifications.inc";


$host = MongoShellServer::getStandaloneInfo();
$ctx  = stream_context_create();
$mc = new MongoClient($host, array(), array("context" => $ctx));

$collection = $mc->selectCollection(dbname(), "write");
$collection->drop();

$mn = new MongoNotifications;
stream_context_set_params($ctx, array("notification" => array($mn, "update")));


for($i = 1; $i <= 10; $i++) {
    echo "Inserting#$i\n";
    $doc = array(
        "example" => "document",
        "with"    => "some",
        "fields"  => "in it",
        "and"     => array(
            "nested", "documents",
        ),
    );

    $opts = array(
        "ordered"  => "somehow",
        "w"        => 1,
        "wTimeoutMS" => 1000,
    );

    $org_opts = $opts;
    $ret = $collection->insert($doc, $opts);
    /* There may be bunch of other fields we don't care for */
    dump_these_keys($ret, array("ok", "n"));
    $meta = $mn->getLastInsertMeta();
    /* Verify the document we actually inserted is identical to the document
     * we wanted in insert.. minus the _id as object vs "$id" column */
    $doc_id = $doc["_id"];
    unset($doc["_id"]);
    $meta_id = $meta["document"]["_id"];
    unset($meta["document"]["_id"]);

    if ($doc_id->{'$id'} ===  $meta_id['$id']) {
        echo "Inserted document#$i had the same ID :)\n";
    } else {
        echo "FAILED ID CHECK!\n";
        var_dump($meta["document"], $meta_id['$id'], $doc, $doc_id->{'$id'});
        throw new Exception("The documents didn't match!");
    }
    
    if ($meta["document"] == $doc) {
        echo "Inserted document#$i was the same as we tried to insert\n";
    } else {
        echo "FAILED META DOCUMENT CHECK\n";
        var_dump($meta["document"], $doc);
        throw new Exception("The documents didn't match!");
    }
    echo "Write Concern:\n";
    var_dump($meta["write_options"]);
    echo "Insert executed in namespace: ", $meta["protocol_options"]["namespace"], "\n";


    $tmp = $collection->findOne(array("_id" => $doc_id));
    unset($tmp["_id"]);
    if ($doc === $tmp) {
        echo "Retrieving the document#$i from DB resulted in the same doc as we inserted too :D\n";
    } else {
        echo "FAILED RETREIVAL CHECK\n";
        var_dump($doc, $tmp);
        throw new Exception("The documents didn't match!");
    }
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Inserting#1
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#1 had the same ID :)
Inserted document#1 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#1 from DB resulted in the same doc as we inserted too :D
Inserting#2
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#2 had the same ID :)
Inserted document#2 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#2 from DB resulted in the same doc as we inserted too :D
Inserting#3
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#3 had the same ID :)
Inserted document#3 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#3 from DB resulted in the same doc as we inserted too :D
Inserting#4
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#4 had the same ID :)
Inserted document#4 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#4 from DB resulted in the same doc as we inserted too :D
Inserting#5
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#5 had the same ID :)
Inserted document#5 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#5 from DB resulted in the same doc as we inserted too :D
Inserting#6
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#6 had the same ID :)
Inserted document#6 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#6 from DB resulted in the same doc as we inserted too :D
Inserting#7
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#7 had the same ID :)
Inserted document#7 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#7 from DB resulted in the same doc as we inserted too :D
Inserting#8
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#8 had the same ID :)
Inserted document#8 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#8 from DB resulted in the same doc as we inserted too :D
Inserting#9
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#9 had the same ID :)
Inserted document#9 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#9 from DB resulted in the same doc as we inserted too :D
Inserting#10
array(2) {
  ["ok"]=>
  float(1)
  ["n"]=>
  int(0)
}
Inserted document#10 had the same ID :)
Inserted document#10 was the same as we tried to insert
Write Concern:
array(2) {
  ["ordered"]=>
  bool(true)
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(1000)
    ["w"]=>
    int(1)
  }
}
Insert executed in namespace: %s.$cmd
Retrieving the document#10 from DB resulted in the same doc as we inserted too :D
===DONE===
