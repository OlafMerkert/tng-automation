### Setup ###

    cd docker
    mkdir node-red-data
    chmod 777 node-red-data
    docker-compose up

### Test ###

    * Install mosquitto client tools (e.g. apt install mosquitto-client)
    * Subscribe to the test topic
        
        mosquitto_sub -t test -h <infrastructure-host>

    * Send a message

        mosquitto_pub -t test -m "Hello wold" -h <infrastructure-host>
