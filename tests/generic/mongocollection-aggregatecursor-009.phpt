--TEST--
MongoCollection::aggregateCursor() with different initial and getmore batch size
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";

function log_query($server, $query, $info) {
    printf("Issuing command: %s\n", key($query));
    if (isset($query['cursor']['batchSize'])) {
        printf("Cursor batch size: %d\n", $query['cursor']['batchSize']);
    }
}

function log_getmore($server, $info) {
    echo "Issuing getmore\n";
    if (isset($info['batch_size'])) {
        printf("Cursor batch size: %d\n", $info['batch_size']);
    }
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_query' => 'log_query',
        'log_getmore' => 'log_getmore',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

for ($i = 0; $i < 103; $i++) {
    $collection->insert(array('article_id' => $i));
}

$cursor = $collection->aggregateCursor(
    array(array('$limit' => 105)),
	array('cursor' => array('batchSize' => 27))
);

$cursor->batchSize(33);

printf("Cursor class: %s\n", get_class($cursor));

foreach ($cursor as $key => $record) {
    echo $key, "\n";
}

?>
===DONE===
--EXPECT--
Issuing command: drop
Cursor class: MongoCommandCursor
Issuing command: aggregate
Cursor batch size: 27
0
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
Issuing getmore
Cursor batch size: 33
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
Issuing getmore
Cursor batch size: 33
60
61
62
63
64
65
66
67
68
69
70
71
72
73
74
75
76
77
78
79
80
81
82
83
84
85
86
87
88
89
90
91
92
Issuing getmore
Cursor batch size: 33
93
94
95
96
97
98
99
100
101
102
===DONE===
