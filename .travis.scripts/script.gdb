set env MALLOC_CHECK_=3
set env ZEND_DONT_UNLOAD_MODULES=1
set env USE_ZEND_ALLOC=0
run
backtrace full
