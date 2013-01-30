<?php
function errorCallBack($c, $m)
{
	echo $m, "\n";
}
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
set_error_handler('errorCallBack');
?>
