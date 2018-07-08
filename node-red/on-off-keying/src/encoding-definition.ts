export interface Pattern {
    value: any;
    pattern: Array<number>;
}

export interface EncodingDefinition {
    patterns:Array<Pattern>;
    deviation:number;
}