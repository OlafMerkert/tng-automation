import {Red, Node} from "node-red";
import {OokDecoder} from "./ook-decoder";
import {EncodingDefinition} from "./encoding-definition";

function registerOokDecodeNode(RED: Red) {
    function OokDecode(config: any) {
        const node: Node = this;
        RED.nodes.createNode(this, config);

        const encodingDefinition: EncodingDefinition = {
            'patterns': [
                {'value': 0, 'pattern': JSON.parse(config.zeroPattern)},
                {'value': 1, 'pattern': JSON.parse(config.onePattern)},
                {'value': 'start', 'pattern': JSON.parse(config.startPattern)}],
            'deviation': parseFloat(config.acceptedDeviation)
        };

        node.on('input', function (msg) {
            let message = msg.payload;
            this.buffer += message;

            if (message.substring(message.length - 1, message.length) !== '+') {
                this.buffer = replaceAll(this.buffer, '+', '');

                let input = replaceAll(this.buffer, '\r', '');
                input = replaceAll(input, '\n', ' ');
                input = input.trim();

                let timings = this.buffer.split(' ')
                    .map((str) => Number(str));

                let bits = OokDecoder.decodeOokMessage(timings, encodingDefinition);
                if (bits && bits.length > 0) {
                    msg.payload = bits;
                    node.send(msg);
                }

                this.buffer = "";
            }
        });
    }

    RED.nodes.registerType('ook_decode', OokDecode);
}

function replaceAll(input: string, oldVal: string, newVal: string): string {
    return input.split(oldVal)
        .join(newVal);
}


export = registerOokDecodeNode;
