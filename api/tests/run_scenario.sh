#!/bin/bash
# This script permits to send messages to API server using the API client, 
# compare responses with excepted result and return if the test passed or not.

# 1st argument corresponds to client's path
# 2nd atgument corresponds to server's path
# 3rd argument is the file path containing messages to send
# 4th argument is the file path containing expected responses
# If there is a 5th parameter, then execute test and write client output to
# expected response file (in verbose mode so you can check response).
if [ -z "$4" ]; then
    echo "Usage: $0 CLIENT_BIN SERVER_BIN REQUEST_FILE EXPECTED_RESPONSE_FILE [LEARN_MODE]"
    echo "if LEARN_MODE is set (any parameter is ok), then client output will be written to"
    echo "EXPECTED_RESPONSE_FILE and client will show response in verbose mode"
    exit 1
fi

client=$1
server=$2
request_file=$3
expected_response=$4
learn_mode=$5

# Check arguments
err=false
if [ ! -f $client ]; then
    echo "client file '$client' not found"
    err=true
fi
if [ ! -f $server ]; then
    echo "server file '$server' not found"
    err=true
fi
if [ ! -f $request_file ]; then
    echo "request file '$request_file' not found"
    err=true
fi

if [ -z $learn_mode ]; then
    if [ ! -f $expected_response ]; then
        echo "expected response file '$expected_response' not found"
        err=true
    fi
fi
if [ $err = true ]; then
    exit 1
fi

# Start server
$server -l debug -i noze -s /tmp/ --endpoint=tcp://0.0.0.0:8765 &
server_pid=$!
sleep 1
kill -s 0 $server_pid
if [ $? -ne 0 ]; then
    echo "server stopped running"
    exit 1
fi

# Prepare client to run
output=/tmp/butterfly_test_api
client_cmd="$client --endpoint=tcp://127.0.0.1:8765 -i $request_file -o $output"
if [ -n "$learn_mode" ]; then
    output=$expected_response
    client_cmd="$client --endpoint=tcp://127.0.0.1:8765 -i $request_file -o $output -v"
fi

# Run client
$client_cmd &
client_pid=$!
sleep 1
kill -s 0 $client_pid &> /dev/null
if [ $? -eq 0 ]; then 
    echo "request process too long"
    exit 1
fi

# Is server still alive ?
kill -s 0 $server_pid
if [ $? -ne 0 ]; then
    echo "server stopped unexpectedly"
    exit 1
fi

# Gently stop server
kill -s 2 $server_pid
kill -s 0 $server_pid &> /dev/null
state=$?
while [ $state -eq 0 ]; do
    kill -s 0 $server_pid &> /dev/null
    state=$?
done

# Compare results
if [ ! -f $output ]; then
    echo "result file not found"
    exit 1
fi
err=false
diff $output $expected_response
if [ $? -ne 0 ]; then
    echo "expected response differs from server's response"
    err=true
fi

# Clean output
if [ -z $learn_mode ]; then
    rm $output
fi

if [ $err = true ]; then
    exit 1
fi

exit 0
