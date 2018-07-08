import {Pattern, EncodingDefinition} from "./encoding-definition";

export class OokDecoder {

    public static decodeOokMessage(input: Array<number>, definition: EncodingDefinition):Array<number> {
        let decodedMessages = OokDecoder.ookDecode(input, definition.patterns, definition.deviation);
        let separateMessages;
        if (definition.patterns.map((pattern) => pattern.value)
            .filter(((value) => value === 'start')).length > 0) {
            separateMessages = this.separateMessagesByHeader(decodedMessages);
            decodedMessages = OokDecoder.ookDecode(input.slice(1), definition.patterns, definition.deviation);
            separateMessages = separateMessages.concat(this.separateMessagesByHeader(decodedMessages));
        } else {
            separateMessages = this.separateMessagesByFooter(decodedMessages);
        }

        return separateMessages.reduce((previousValue: Array<number>, currentValue: Array<number>) => {
                return previousValue.length > currentValue.length ? previousValue : currentValue;
            }
            , []);
    }

    private static separateMessagesByHeader(input: Array<any>): Array<Array<number>> {
        let result: number[][] = [];

        let currentMessage: number[] = [];
        let startBitFound = false;

        input.forEach((bit, index) => {
            if (bit === "start") {
                startBitFound = true;
            }
            else if (bit !== "fail") {
                if (startBitFound) {
                    currentMessage.push(bit);
                }
            }

            if (bit == 'fail' || index >= input.length - 1) {
                if (currentMessage.length > 0) {

                    let currentMessageStr = currentMessage.toString();

                    if (result.filter((msg) => msg.toString() === currentMessageStr).length === 0) {
                        result.push(currentMessage);
                    }
                }
                currentMessage = [];
                startBitFound = false;
            }
        });

        return result;
    }

    private static separateMessagesByFooter(input: Array<any>): Array<Array<number>> {
        let result: number[][] = [];

        let currentMessage: number[] = [];
        let startBitFound = false;

        input.forEach((bit) => {
            if (bit === "footer") {
                if (result.filter((msg) => msg.toString() === currentMessage.toString()).length === 0) {
                    result.push(currentMessage);
                }
                currentMessage = [];
            }
            else if (bit === "fail") {
                currentMessage = [];
            } else {
                currentMessage.push(bit);
            }
        });

        return result;
    }

    private static ookDecode(input: Array<number>, patterns: Array<Pattern>, allowedDeviation: number): Array<any> {
        let result = [];

        let i = 0;
        while (i < input.length) {
            let matchResult = OokDecoder.findMatchingPattern(input, patterns, allowedDeviation);

            if (matchResult) {
                result.push(matchResult.value);
                input = input.slice(matchResult.pattern.length);
            }
            else {
                result.push("fail");
                input = input.slice(1);
            }
        }
        return result;
    }

    private static findMatchingPattern(input: Array<number>, patterns: Array<Pattern>, allowedDeviation: number): Pattern {
        for (let pattern of patterns) {
            let comparedInput = input.slice(0, pattern.pattern.length);
            if (OokDecoder.matchesPattern(comparedInput, pattern, allowedDeviation)) {
                return (pattern);
            }
        }
        return;
    }

    private static matchesPattern(input: Array<number>, pattern: Pattern, allowedDeviation: number): boolean {
        for (let i = 0; i < pattern.pattern.length; i++) {
            if (!OokDecoder.isInRange(input[i], pattern.pattern[i], allowedDeviation)) {
                return false;
            }
        }
        return true;
    }

    private static isInRange(value: number, target: number, allowedDeviation: number) {
        return (value > target * (1 - allowedDeviation) && value < target * (1 + allowedDeviation));
    }

}