import {Node, Red} from "node-red";
import {sanitizeTimingString} from "./utils";

function registerOokConcatNode(RED: Red) {
    function OokConcat(config: any) {
        const node: Node = this;
        RED.nodes.createNode(this, config);

        node.on('input', function (msg) {
            let message = msg.payload;
            this.buffer += message;

            if (message.substring(message.length - 1, message.length) !== '+') {

                msg.payload = sanitizeTimingString(this.buffer);
                node.send(msg);

                this.buffer = "";
            }
        });
    }

    RED.nodes.registerType('ook_concat', OokConcat);
}

export = registerOokConcatNode;
