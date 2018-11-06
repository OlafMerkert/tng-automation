import {Node} from "node-red";

export function replaceAll(input: string, oldVal: string, newVal: string): string {
    return input.split(oldVal)
        .join(newVal);
}

export function sanitizeTimingString(input) {
    input = replaceAll(input, '+', '');
    input = replaceAll(input, '\r', '');
    input = replaceAll(input, '\n', ' ');
    return input.trim();
}

export function parseTimings(timingString) {
    return timingString.split(' ')
        .map((str) => Number(str));
}

export function sendTimingsInChunks(timings, node: Node) {
    let i, j, chunk = 30;
    for (i = 0, j = timings.length; i < j; i += chunk) {
        let timingChunk = timings.slice(i, i + chunk);

        let msg:any={};
        msg.payload = timingChunk.join(' ');

        if (i < j - chunk) {
            msg.payload += ' +';
        }

        node.send(msg);
    }
}