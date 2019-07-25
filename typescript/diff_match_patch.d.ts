export default class diff_match_patch {
    Diff_Timeout: number;
    Diff_EditCost: number;
    Match_Threshold: number;
    Match_Distance: number;
    Patch_DeleteThreshold: number;
    Patch_Margin: number;
    Match_MaxBits: number;
    constructor();
    diff_main(text1: string, text2: string, opt_checklines?: boolean, opt_deadline?: number): Diff[];
    private diff_compute_;
    private diff_lineMode_;
    private diff_bisect_;
    private diff_bisectSplit_;
    private diff_linesToChars_;
    private diff_charsToLines_;
    diff_commonPrefix(text1: string, text2: string): number;
    diff_commonSuffix(text1: string, text2: string): number;
    private diff_commonOverlap_;
    private diff_halfMatch_;
    diff_cleanupSemantic(diffs: Diff[]): void;
    diff_cleanupSemanticLossless(diffs: Diff[]): void;
    private static nonAlphaNumericRegex_;
    private static whitespaceRegex_;
    private static linebreakRegex_;
    private static blanklineEndRegex_;
    private static blanklineStartRegex_;
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
    private match_bitap_;
    private match_alphabet_;
    private patch_addContext_;
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
export declare class Diff {
    operation: number;
    text: string;
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
