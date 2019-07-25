export declare class diff_match_patch {
    Diff_Timeout: number;
    Diff_EditCost: number;
    Match_Threshold: number;
    Match_Distance: number;
    Patch_DeleteThreshold: number;
    Patch_Margin: number;
    Match_MaxBits: number;
    constructor();
    diff_main(text1: string, text2: string, opt_checklines?: boolean, opt_deadline?: number): Diff[];
    diff_compute_(text1: string, text2: string, checklines: boolean, deadline: number): Diff[];
    diff_lineMode_(text1: string, text2: string, deadline: number): Diff[];
    diff_bisect_(text1: string, text2: string, deadline: number): Diff[];
    diff_bisectSplit_(text1: string, text2: string, x: number, y: number, deadline: number): Diff[];
    diff_linesToChars_(text1: string, text2: string): {
        chars1: string;
        chars2: string;
        lineArray: string[];
    };
    diff_charsToLines_(diffs: Diff[], lineArray: string[]): void;
    diff_commonPrefix(text1: string, text2: string): number;
    diff_commonSuffix(text1: string, text2: string): number;
    diff_commonOverlap_(text1: string, text2: string): number;
    diff_halfMatch_(text1: string, text2: string): string[] | null;
    diff_cleanupSemantic(diffs: Diff[]): void;
    diff_cleanupSemanticLossless(diffs: Diff[]): void;
    static nonAlphaNumericRegex_: RegExp;
    static whitespaceRegex_: RegExp;
    static linebreakRegex_: RegExp;
    static blanklineEndRegex_: RegExp;
    static blanklineStartRegex_: RegExp;
    diff_cleanupEfficiency(diffs: Diff[]): void;
    diff_cleanupMerge(diffs: Diff[]): void;
    diff_xIndex(diffs: Diff[], loc: number): number;
    diff_prettyHtml(diffs: Diff[]): string;
    diff_text1(diffs: Diff[]): string;
    diff_text2(diffs: Diff[]): string;
    diff_levenshtein(diffs: Diff[]): number;
    diff_toDelta(diffs: Diff[]): string;
    diff_fromDelta(text1: string, delta: string): Diff[];
    match_main(text: string, pattern: string, loc: number): number;
    match_bitap_(text: string, pattern: string, loc: number): number;
    match_alphabet_(pattern: string): {
        [key: string]: number;
    };
    patch_addContext_(patch: patch_obj, text: string): void;
    patch_make(a: string | Diff[], opt_b?: string | Diff[], opt_c?: string | Diff[]): patch_obj[];
    patch_deepCopy(patches: patch_obj[]): patch_obj[];
    patch_apply(patches: patch_obj[], text: string): (string | boolean[])[];
    patch_addPadding(patches: patch_obj[]): string;
    patch_splitMax(patches: patch_obj[]): void;
    patch_toText(patches: patch_obj[]): string;
    patch_fromText(textline: string): patch_obj[];
}
export declare const DIFF_DELETE = -1;
export declare const DIFF_INSERT = 1;
export declare const DIFF_EQUAL = 0;
export interface IDiff<T0, T1> {
    0: T0;
    1: T1;
}
export declare class Diff implements IDiff<number, string> {
    0: number;
    1: string;
    constructor(op: number, text: string);
    toString(): string;
}
export declare class patch_obj {
    diffs: Diff[];
    start1: number | null;
    start2: number | null;
    length1: number;
    length2: number;
    constructor();
    toString(): string;
}
