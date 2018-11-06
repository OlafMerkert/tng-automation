import {Node, Red} from "node-red";
import {parseTimings, sanitizeTimingString, sendTimingsInChunks} from "./utils";

function registerOokSplitNode(RED: Red) {
    function OokSplit(config: any) {
        const node: Node = this;
        RED.nodes.createNode(this, config);
        this.buffer = "";

        node.on('input', function (msg) {
            let message = msg.payload;
            this.buffer += message;

            if (message.substring(message.length - 1, message.length) !== '+') {
                let timings = parseTimings(sanitizeTimingString(this.buffer));
                sendTimingsInChunks(timings, node);

                this.buffer = "";
            }
        });
    }

    RED.nodes.registerType('ook_split', OokSplit);
}

export = registerOokSplitNode;
