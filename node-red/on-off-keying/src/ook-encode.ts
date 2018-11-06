import {Red, Node} from "node-red";
import {EncodingDefinition} from "./encoding-definition";
import {sendTimingsInChunks} from "./utils";

function registerOokEncodeNode(RED: Red) {
    function OokEncode(config: any) {
        const node: Node = this;
        RED.nodes.createNode(this, config);

        const encodingDefinition: EncodingDefinition = {
            'patterns': [
                {'value': 0, 'pattern': JSON.parse(config.zeroPattern)},
                {'value': 1, 'pattern': JSON.parse(config.onePattern)},
                {'value': 'start', 'pattern': JSON.parse(config.startPattern)}],
            'deviation': undefined
        };

        function encodeOokMessage(input: Array<number>, definition: EncodingDefinition): Array<number> {
            let patterns = [getPattern('start', definition)];
            patterns = patterns.concat(input.map(value => getPattern(value, definition)));

            let result = [];

            patterns.forEach(pattern => pattern.forEach(value => result.push(value)));

            return result;
        }

        function getPattern(value: any, definition: EncodingDefinition) {
            let matchingPatterns = definition.patterns.filter(pattern => pattern.value == value);
            if (matchingPatterns.length === 0) {
                node.error(`no matching pattern for value ${value}`);
                return;
            }
            return matchingPatterns[0].pattern;
        }

        node.on('input', function (msg) {
            let input = JSON.parse(msg.payload);
            let timings = encodeOokMessage(input, encodingDefinition);
            sendTimingsInChunks(timings, node);
        });
    }

    RED.nodes.registerType('ook_encode', OokEncode);
}

export = registerOokEncodeNode;
